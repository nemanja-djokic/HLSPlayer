#include "headers/TSVideo.h"
#include <fstream>
#include <ios>

TSVideo::TSVideo(std::string fname)
{
    this->_fname = fname;
}

void TSVideo::appendData(uint8_t* buffer, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_videoPayload.insert(this->_videoPayload.end(), buffer[i]);
        this->_hasData = true;
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
    std::ofstream tmpfile;
    tmpfile.open("/dev/shm/" + _fname, std::ios::out | std::ios::binary);
    tmpfile.write((char*)this->getPayload(), this->getSize());
    tmpfile.close();
    this->_isSaved = true;
}