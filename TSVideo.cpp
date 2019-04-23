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
    this->_fname = fname;
    this->_formatContext = nullptr;
    this->_ioCtx = nullptr;
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

void TSVideo::appendAudio(uint8_t* buffer, size_t len)
{
    uint8_t* helpBuffer = (uint8_t*)av_malloc(_ioCtx->_audioBufferSize + len);
    memcpy(helpBuffer, _ioCtx->_audioBuffer, _ioCtx->_audioBufferSize);
    memcpy(helpBuffer + _ioCtx->_audioBufferSize, buffer, len);
    av_free(_ioCtx->_audioBuffer);
    _ioCtx->_audioBuffer = helpBuffer;
    _ioCtx->_audioBufferSize += len;
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

void TSVideo::prepareFile()
{
    size_t size = this->_videoPayload.size();
    uint8_t* payload = new uint8_t[size];
    std::copy(_videoPayload.begin(), _videoPayload.end(), payload);
    this->_ioCtx = new CustomIOContext();
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
            while(pos < (int64_t)this->_tsBlockBegin->size() && this->_tsBlockBegin->at(++pos) < this->_ioCtx->_pos);
            pos--;
            if(pos < 0)
            {
                this->_ioCtx->_ioCtx->seek(this->_ioCtx, 0, SEEK_SET);
                return;
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
                    while(pos > -1 && fabs(toSeek) > this->_tsBlockDuration->at(pos))
                    {
                        toSeek += this->_tsBlockDuration->at(pos);
                        pos--;
                    }
                    if(pos < 0)
                    {
                        this->_ioCtx->_ioCtx->seek(this->_ioCtx, 0, SEEK_SET);
                        return;
                    }
                    int64_t offsetBytes = (int64_t)((double)this->_tsBlockSize->at(pos) * (fabs(toSeek) / currentBlockDuration)); 
                    this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(pos + 1) - offsetBytes, SEEK_SET);
                }
            }
            /*if(offset < 0)
            {
                double toSeek = (double)offset;
                toSeek -= currentBlockElapsed;
                int posOfEndBlock = pos;
                while(posOfEndBlock > 0 && toSeek < 0)
                {
                    toSeek += this->_tsBlockDuration->at(posOfEndBlock);
                    posOfEndBlock--;
                }
                if(posOfEndBlock < 0)
                {
                    posOfEndBlock = 0;
                }
                else if(toSeek > 0 && fabs(toSeek) < (this->_tsBlockDuration->at(posOfEndBlock)) 
                && fabs(toSeek) > (this->_tsBlockDuration->at(posOfEndBlock)/2))
                {
                    posOfEndBlock+=1;
                }
                this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(posOfEndBlock), SEEK_SET);
                this->_ioCtx->_resetAudio = true;
            }
            else if(offset > 0)
            {
                double toSeek = (double)offset;
                toSeek += currentBlockElapsed;
                int posOfEndBlock = pos;
                while((uint32_t)posOfEndBlock < this->_tsBlockBegin->size() && toSeek > 0)
                {
                    toSeek -= this->_tsBlockDuration->at(posOfEndBlock);
                    posOfEndBlock++;
                }
                posOfEndBlock--;
                if((uint32_t)posOfEndBlock > this->_tsBlockBegin->size() - 1)
                {
                    posOfEndBlock = this->_tsBlockBegin->size() - 1;
                }
                else if(toSeek > 0 && fabs(toSeek) < (this->_tsBlockDuration->at(posOfEndBlock)) 
                && fabs(toSeek) > (this->_tsBlockDuration->at(posOfEndBlock)/2))
                {
                    posOfEndBlock+=1;
                }
                this->_ioCtx->_resetAudio = true;
                this->_ioCtx->_ioCtx->seek(this->_ioCtx, this->_tsBlockBegin->at(posOfEndBlock), SEEK_SET);
            }*/
            avio_flush(this->_ioCtx->_ioCtx);
            if(_videoCodec != nullptr)
            {
                //avcodec_flush_buffers(_videoCodec);
            }
            else
            {
                std::cerr << "(TSVideo) Video codec unassigned" << std::endl;
            }
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

void TSVideo::refreshTimer(AVPacket packet)
{
    this->_currentPts = packet.pts;
    this->_currentPtsTime = (int64_t)(av_q2d(av_get_time_base_q()));
}

double TSVideo::getVideoClock()
{
    double delta;
    delta = ( (int64_t)(av_q2d(av_get_time_base_q())) - this->_currentPtsTime ) / 1000000.0;
    return this->_currentPtsTime + delta;
}

double TSVideo::getExternalClock()
{
    return av_gettime() / 1000000.0;
}

double TSVideo::synchronizeVideo(AVFrame* sourceFrame, double pts)
{
    double frameDelay;
    if(pts != 0)
    {
        this->_currentPts = pts;
    }
    else
    {
        pts = this->_currentPts;
    }
    frameDelay = av_q2d(this->_videoCodec->time_base);
    frameDelay += sourceFrame->repeat_pict * (frameDelay * 0.5);
    this->_currentPts += frameDelay;
    return pts;
}

int32_t TSVideo::synchronizeAudio(uint32_t samplesSize, double pts)
{
    int n;
    double refClock;
    n = 2 * this->_audioCodec->channels;

    double diff, avgDiff;
    int wantedSize, minSize, maxSize, nbSamples;

    minSize = samplesSize / 2;
    maxSize = samplesSize * 1.5;

    refClock = getExternalClock();
    diff = (this->_currentPts - pts) / 10000;

    _differenceCumulative += diff;
    _differenceCounter++;

    if(fabs(_differenceCumulative) < 150)
    {
        wantedSize = samplesSize;
    }
    else
    {
        int32_t samplesDifference = (_differenceCumulative * this->_audioCodec->sample_rate) / 1000;
        wantedSize = samplesSize - samplesDifference;
        wantedSize = (wantedSize > maxSize)?maxSize : wantedSize;
        wantedSize = (wantedSize < minSize)?minSize : wantedSize;
        _differenceCumulative = 0;
        _differenceCounter = 0;
    }
    return wantedSize;
}