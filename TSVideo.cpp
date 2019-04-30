#include "headers/TSVideo.h"

extern "C"
{
    #include "libavutil/time.h"
}

#include <fstream>
#include <ios>
#include <iostream>
#include <cstdlib>

TSVideo::TSVideo(std::string fname)
{
    this->_ioCtx = new CustomIOContext();
    this->_fname = fname;
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

void TSVideo::appendData(uint8_t* buffer, size_t len, bool isFirst, bool wholeBlock, double duration)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_videoPayload.insert(this->_videoPayload.end(), buffer[i]);
        this->_hasData = true;
    }
    SDL_LockMutex(_ioCtx->_bufferMutex);
    _blockBufferSize += len;
    if(wholeBlock)
    {
        this->_tsBlockDuration->push_back(duration);
    }
    if(!isFirst && wholeBlock)
    {
        uint8_t* helpBuffer = new uint8_t[_videoPayload.size()];
        std::copy(_videoPayload.begin(), _videoPayload.end(), helpBuffer);
        this->_ioCtx->_videoBuffer = (uint8_t*)av_realloc(this->_ioCtx->_videoBuffer, _videoPayload.size());
        memcpy(this->_ioCtx->_videoBuffer, helpBuffer, _videoPayload.size());
        this->_ioCtx->_videoBufferSize = _videoPayload.size();
        this->_tsBlockBegin->push_back(_ioCtx->_videoBufferSize);
        delete[] helpBuffer;
    }
    SDL_UnlockMutex(_ioCtx->_bufferMutex);
}

void TSVideo::finalizeLoading()
{
    uint8_t* helpBuffer = new uint8_t[_videoPayload.size()];
    std::copy(_videoPayload.begin(), _videoPayload.end(), helpBuffer);
    this->_ioCtx->_videoBuffer = (uint8_t*)av_realloc(this->_ioCtx->_videoBuffer, _videoPayload.size());
    memcpy(this->_ioCtx->_videoBuffer, helpBuffer, _videoPayload.size());
    this->_ioCtx->_videoBufferSize = _videoPayload.size();
    delete[] helpBuffer;
}


void TSVideo::sizeAccumulate()
{
    this->_tsBlockSize->push_back(_blockBufferSize);
    _blockBufferSize = 0;
}

uint8_t* TSVideo::getPayload()
{
    uint8_t* out = new uint8_t[this->_videoPayload.size()];
    std::copy(_videoPayload.begin(), _videoPayload.end(), out);
    return out;
}

uint32_t TSVideo::getFullDuration()
{
    double total = 0.0;
    for(size_t i = 0; i < _tsBlockDuration->size(); i++)
    {
        total += _tsBlockDuration->at(i);
    }
    return (uint32_t)total;
}

void TSVideo::prepareFile()
{
    size_t size = this->_videoPayload.size();
    uint8_t* payload = new uint8_t[size];
    std::copy(_videoPayload.begin(), _videoPayload.end(), payload);
    this->_ioCtx->_videoBuffer = (uint8_t*)av_malloc(_videoPayload.size());
    memcpy(this->_ioCtx->_videoBuffer, payload, _videoPayload.size());
    this->_ioCtx->_videoBufferSize = _videoPayload.size();
    delete[] payload;
}

void TSVideo::prepareFormatContext(AVFormatContext* oldFormatContext)
{
    if(oldFormatContext == nullptr)
    {
        oldFormatContext = avformat_alloc_context();
    }
    oldFormatContext->pb = this->_ioCtx->_ioCtx;
    oldFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    int err = avformat_open_input(&oldFormatContext, "", nullptr, nullptr);
    if(err != 0)
    {
        std::cerr << "Format context failed" << std::endl;
        _formatContext = nullptr;
    }
    _formatContext = oldFormatContext;
    if(avformat_find_stream_info(_formatContext, nullptr) < 0)
    {
        std::cerr << "Stream info failed" << std::endl;
        _formatContext = nullptr;
    }
}

void TSVideo::seek(int64_t offset, int64_t whence)
{
    if(this->_ioCtx != nullptr)
    {
        if(whence == SEEK_CUR)
        {
            SDL_LockMutex(_videoPlayerMutex);
            int pos = -1;
            while(pos < (int64_t)this->_tsBlockBegin->size() - 1 && this->_tsBlockBegin->at(++pos) < this->_ioCtx->_pos);
            pos--;
            if(pos < 0)
            {
                this->_ioCtx->_ioCtx->seek(this->_ioCtx, 0, SEEK_SET);
                return;
            }
            else if(pos > (int64_t)_tsBlockBegin->size() - 2)
            {
                if(offset < 0)
                {
                    this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(this->_tsBlockBegin->size() - 1), SEEK_SET);
                    return;
                }
                else if(offset > 0)
                {
                    return;
                }
            }
            double currentBlockDuration = this->_tsBlockDuration->at(pos);
            double currentBlockElapsed = ((double)(this->_ioCtx->_pos - this->_tsBlockBegin->at(pos)) / (double)this->_tsBlockSize->at(pos))
            *this->_tsBlockDuration->at(pos);
            if(offset > 0)
            {
                double toSeek = (double)offset;
                if(toSeek < (currentBlockDuration - currentBlockElapsed))
                {
                    int64_t offsetBytes = (int64_t)((double)this->_tsBlockSize->at(pos) * (toSeek / currentBlockDuration));
                    this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos) + offsetBytes, SEEK_SET);
                }
                else
                {
                    toSeek -= (currentBlockDuration - currentBlockElapsed);
                    pos++;
                    while(toSeek > this->_tsBlockDuration->at(pos))
                    {
                        toSeek -= this->_tsBlockDuration->at(pos);
                        pos++;
                    }
                    int64_t offsetBytes = (int64_t)((double)this->_tsBlockSize->at(pos) * (toSeek / currentBlockDuration));
                    this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos) + offsetBytes, SEEK_SET);
                }
            }
            else if(offset < 0)
            {
                double toSeek = (double)offset - 1;
                if(fabs(toSeek) < currentBlockElapsed)
                {
                    int64_t offsetBytes = (int64_t)((double)this->_tsBlockSize->at(pos) * (fabs(toSeek) / currentBlockDuration));
                    this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos) - offsetBytes, SEEK_SET);
                }
                else
                {
                    toSeek += currentBlockElapsed;
                    pos--;
                    while(pos > -1 && pos < (int64_t)this->_tsBlockDuration->size() && fabs(toSeek) > this->_tsBlockDuration->at(pos))
                    {
                        toSeek += this->_tsBlockDuration->at(pos);
                        pos--;
                    }
                    if(pos < 0)
                    {
                        this->_ioCtx->_ioCtx->seek(this->_ioCtx, 0, SEEK_SET);
                        return;
                    }
                    if(pos >= (int64_t)_tsBlockDuration->size())pos--;
                    int64_t offsetBytes = (int64_t)((double)this->_tsBlockSize->at(pos) * (fabs(toSeek) / currentBlockDuration)); 
                    if(pos < (int64_t)_tsBlockSize->size() - 1)this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos + 1) - offsetBytes, SEEK_SET);
                    else this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos), SEEK_SET);
                }
            }
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
            int32_t totalSize = 0;
            for(size_t i = 0; i < this->_tsBlockSize->size(); i++)
                totalSize += this->_tsBlockSize->at(i);
            int32_t newPos = (offset * totalSize) / 10;
            this->_ioCtx->_ioCtx->seek(this->_ioCtx, newPos, SEEK_SET);
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
        cumulative += this->_tsBlockDuration->at(pos);
        lastAdded = this->_tsBlockDuration->at(pos);
    }
    cumulative -= lastAdded;
    pos--;
    if(pos < 0)
    {
       return 0;
    }
    double currentBlockElapsed = ((double)(this->_ioCtx->_pos - this->_tsBlockBegin->at(pos)) / (double)this->_tsBlockSize->at(pos))
    *this->_tsBlockDuration->at(pos);
    return (uint32_t)(cumulative + currentBlockElapsed);
}