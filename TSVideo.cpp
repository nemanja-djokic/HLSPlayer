#include "headers/TSVideo.h"

extern "C"
{
    #include "libavutil/time.h"
}

#include <fstream>
#include <ios>
#include <iostream>
#include <cstdlib>
#include <iomanip>

TSVideo::TSVideo()
{
    this->_ioCtx = new CustomIOContext();
    this->_formatContext = nullptr;
    this->_videoCodec = nullptr;
    this->_audioCodec = nullptr;
    _lastTimestamp = -1;
    _videoQueue = new std::queue<AVPacket>();
    _audioQueue = new std::queue<AVPacket>();
    _tsBlockBegin = new std::vector<int32_t>();
    _tsBlockBegin->push_back(0);
    _tsBlockDuration = new std::vector<double>();
    _tsBlockSize = new std::vector<int32_t>();
    _blockBufferSize = 0;
    _videoPlayerMutex = SDL_CreateMutex();
    _videoQueueMutex = SDL_CreateMutex();
    _audioQueueMutex = SDL_CreateMutex();
    _differenceCumulative = 0.0;
    _differenceCounter = 0;
    font = TTF_OpenFont("/usr/share/fonts/truetype/Sarai/Sarai.ttf", 72);
    if (font == nullptr)
    {
        std::cerr << "TTF_OpenFont() Failed: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }
}

TSVideo::~TSVideo()
{
    std::cout << "Called TSVideo DTOR" << std::endl;
    delete _tsBlockBegin;
    delete _tsBlockDuration;
    delete _tsBlockSize;
    delete _videoQueue;
    delete _audioQueue;
}

uint32_t TSVideo::getFullDuration()
{
    double total = 0.0;
    for(int32_t i = 0; i < this->_ioCtx->_networkManager->getSegmentsSize(); i++)
    {
        total += this->_ioCtx->_networkManager->getBlockDuration(i);
    }
    return (uint32_t)total;
}

void TSVideo::prepareFormatContext()
{
    _formatContext = avformat_alloc_context();
    _formatContext->pb = this->_ioCtx->_ioCtx;
    _formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    int err = avformat_open_input(&_formatContext, "", nullptr, nullptr);
    if(err != 0)
    {
        std::cerr << "Format context failed" << std::endl;
        _formatContext = nullptr;
    }
    if(avformat_find_stream_info(_formatContext, nullptr) < 0)
    {
        std::cerr << "Stream info failed" << std::endl;
        _formatContext = nullptr;
    }
}

void TSVideo::seek(int64_t offset, int64_t whence, int64_t currentTimestamp)
{
    if(this->_ioCtx != nullptr)
    {
        if(whence == SEEK_CUR)
        {
            double currentBlockElapsed = this->_ioCtx->_pos * this->_ioCtx->_networkManager->getBlockDuration(this->_ioCtx->_block) 
            / this->_ioCtx->_networkManager->getSegment(this->_ioCtx->_block)->loadedSize();
            double offsetFromCurrent = offset + currentBlockElapsed;
            int64_t selectedBlock = this->_ioCtx->_block;
            if(offsetFromCurrent < 0)
            {
                while(selectedBlock > 0 && this->_ioCtx->_networkManager->getBlockDuration(selectedBlock) < fabs(offsetFromCurrent))
                {
                    offsetFromCurrent += this->_ioCtx->_networkManager->getBlockDuration(selectedBlock);
                    selectedBlock--;
                }
                offsetFromCurrent = (offsetFromCurrent < 0.0)? 0.0:offsetFromCurrent;
            }
            else
            {
                while(selectedBlock < (int64_t)this->_ioCtx->_networkManager->getSegmentsSize() - 1 && this->_ioCtx->_networkManager->getBlockDuration(selectedBlock) < offsetFromCurrent)
                {
                    offsetFromCurrent -= this->_ioCtx->_networkManager->getBlockDuration(selectedBlock);
                    selectedBlock++;
                }
                selectedBlock = (selectedBlock > (int64_t)this->_ioCtx->_networkManager->getSegmentsSize() - 1)?(int64_t)this->_ioCtx->_networkManager->getSegmentsSize():selectedBlock;
            }
            int64_t posOffset = 0;
            if(!this->_ioCtx->_networkManager->getSegment(selectedBlock)->getIsLoaded())
                this->_ioCtx->_networkManager->getSegment(selectedBlock)->loadSegment();
            if(this->_ioCtx->_networkManager->getSegment(selectedBlock)->getIsLoaded())
            {
                if(offsetFromCurrent < 0)
                {
                    posOffset = ((this->_ioCtx->_networkManager->getBlockDuration(selectedBlock) + offsetFromCurrent) / this->_ioCtx->_networkManager->getBlockDuration(selectedBlock))
                        * this->_ioCtx->_networkManager->getSegment(selectedBlock)->loadedSize();
                }
                else
                {
                    posOffset = (offsetFromCurrent / this->_ioCtx->_networkManager->getBlockDuration(selectedBlock))
                        * this->_ioCtx->_networkManager->getSegment(selectedBlock)->loadedSize();
                }
            }
            if(offsetFromCurrent > this->_ioCtx->_networkManager->getBlockDuration(selectedBlock)/2 && selectedBlock < (int64_t)this->_ioCtx->_networkManager->getSegmentsSize() - 1)
                selectedBlock++;
            this->_ioCtx->_blockToSeek = selectedBlock;
            this->_ioCtx->_ioCtx->seek(this->_ioCtx, posOffset, SEEK_SET);
            avio_flush(this->_ioCtx->_ioCtx);
            if(_audioCodec != nullptr)
            {
                avcodec_flush_buffers(_audioCodec);
            }
            else
            {
                std::cerr << "(TSVideo) Audio codec unassigned" << std::endl;
            }
            SDL_UnlockMutex(_videoPlayerMutex);
        }
        else if(whence == SEEK_SET)
        {
            int64_t seekTimestamp = (this->getFullDuration() * offset) / 10;
            this->seek(seekTimestamp, SEEK_DATA, currentTimestamp);
        }
        else if(whence == SEEK_DATA)
        {
            SDL_LockMutex(_videoPlayerMutex);
            double pos = (double)offset;
            int64_t selectedBlock = -1;
            for(int32_t i = 0; i < this->_ioCtx->_networkManager->getSegmentsSize(); i++)
            {
                if(pos < this->_ioCtx->_networkManager->getBlockDuration(i))
                {
                    selectedBlock = i;
                    break;
                }
                pos -= this->_ioCtx->_networkManager->getBlockDuration(i);
            }
            if(selectedBlock == -1)
                return;
            if(!this->_ioCtx->_networkManager->getSegment(selectedBlock)->getIsLoaded())
                this->_ioCtx->_networkManager->getSegment(selectedBlock)->loadSegment();
            this->_ioCtx->_blockToSeek = selectedBlock;
            int64_t seekPos = (pos * 1000) / this->_ioCtx->_networkManager->getBlockDuration(selectedBlock);
            seekPos *= this->_ioCtx->_networkManager->getSegment(selectedBlock)->loadedSize();
            seekPos /= 1000;
            this->_ioCtx->_ioCtx->seek(this->_ioCtx, seekPos, SEEK_SET);
            avio_flush(this->_ioCtx->_ioCtx);
            if(_audioCodec != nullptr)
            {
                avcodec_flush_buffers(_audioCodec);
            }
            else
            {
                std::cerr << "(TSVideo) Audio codec unassigned" << std::endl;
            }
            SDL_UnlockMutex(_videoPlayerMutex);
        }
    }
}

uint32_t TSVideo::getSeconds()
{
    int pos = -1;
    double cumulative = 0.0;
    double lastAdded = 0.0;
    while(pos < (int64_t)this->_tsBlockBegin->size() - 1 && this->_tsBlockBegin->at(++pos) < this->_ioCtx->_pos){
        cumulative += this->_ioCtx->_networkManager->getBlockDuration(pos);
        lastAdded = this->_ioCtx->_networkManager->getBlockDuration(pos);
    }
    cumulative -= lastAdded;
    pos--;
    if(pos < 0)
    {
       return 0;
    }
    double currentBlockElapsed = ((double)(this->_ioCtx->_pos - this->_tsBlockBegin->at(pos)) / (double)this->_tsBlockSize->at(pos))
    *this->_ioCtx->_networkManager->getBlockDuration(pos);
    return (uint32_t)(cumulative + currentBlockElapsed);
}