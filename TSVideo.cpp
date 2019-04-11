#include "headers/TSVideo.h"
#include <fstream>
#include <ios>
#include <iostream>

TSVideo::TSVideo(std::string fname)
{
    this->_fname = fname;
    this->_formatContext = nullptr;
    this->_ioCtx = nullptr;
}

void TSVideo::appendData(uint8_t* buffer, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_videoPayload.insert(this->_videoPayload.end(), buffer[i]);
        this->_hasData = true;
    }
    /*if(this->_ioCtx == nullptr)
    {
        std::cout << "Here1" << std::endl;
        size_t size = this->_videoPayload.size();
        uint8_t* payload = new uint8_t[size];
        std::copy(_videoPayload.begin(), _videoPayload.end(), payload);
        _ioCtx = new CustomIOContext(payload, size);
        std::cout << "Here2" << std::endl;
    }
    else
    {
        SDL_LockMutex(_ioCtx->_ioCtxMutex);
        std::cout << "Here3" << std::endl;
        uint8_t* tempBuffer = new uint8_t[this->_videoPayload.size()];
        std::copy(this->_videoPayload.begin(), this->_videoPayload.end(), tempBuffer);
        //if(_ioCtx->_buffer != nullptr)delete[] _ioCtx->_buffer;
        _ioCtx->_buffer = tempBuffer;
        _ioCtx->_bufferSize = _videoPayload.size();
        std::cout << "Here4" << std::endl;
        SDL_UnlockMutex(_ioCtx->_ioCtxMutex);
    }*/
}

uint8_t* TSVideo::getPayload()
{
    uint8_t* out = new uint8_t[this->_videoPayload.size()];
    std::copy(_videoPayload.begin(), _videoPayload.end(), out);
    return out;
}

void TSVideo::prepareFile()
{
    /*std::ofstream tmpfile;
    tmpfile.open("/dev/shm/" + _fname, std::ios::out | std::ios::binary);
    tmpfile.write((char*)this->getPayload(), this->getSize());
    tmpfile.close();
    this->_isSaved = true;*/
    size_t size = this->_videoPayload.size();
    uint8_t* payload = new uint8_t[size];
    std::copy(_videoPayload.begin(), _videoPayload.end(), payload);
    this->_ioCtx = new CustomIOContext(payload, size);
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
        char* buff = new char[16386];
        std::cout << av_strerror(err, buff, 16386) << std::endl;
        std::cout << buff << std::endl;
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