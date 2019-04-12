#include "headers/TSVideo.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <cstdlib>

TSVideo::TSVideo(std::string fname)
{
    this->_fname = fname;
    this->_formatContext = nullptr;
    this->_ioCtx = nullptr;
}

void TSVideo::appendData(uint8_t* buffer, size_t len, bool isFirst, bool wholeBlock)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_videoPayload.insert(this->_videoPayload.end(), buffer[i]);
        this->_hasData = true;
    }
    if(!isFirst && wholeBlock)
    {
        uint8_t* helpBuffer = new uint8_t[_videoPayload.size()];
        std::copy(_videoPayload.begin(), _videoPayload.end(), helpBuffer);
        this->_ioCtx->_videoBuffer = (uint8_t*)av_realloc(this->_ioCtx->_videoBuffer, _videoPayload.size());
        memcpy(this->_ioCtx->_videoBuffer, helpBuffer, _videoPayload.size());
        this->_ioCtx->_videoBufferSize = _videoPayload.size();
        delete[] helpBuffer;
    }
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
        this->_ioCtx->_ioCtx->seek(this->_ioCtx, offset, whence);
}