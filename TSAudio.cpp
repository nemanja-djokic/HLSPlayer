#include "headers/TSAudio.h"
#include <fstream>
#include <ios>

TSAudio::TSAudio(std::string fname)
{
    this->_fname = fname;
}

void TSAudio::appendData(uint8_t* buffer, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_audioPayload.insert(this->_audioPayload.end(), buffer[i]);
        this->_hasData = true;
    }
}

uint8_t* TSAudio::getPayload()
{
    uint8_t* out = new uint8_t[this->_audioPayload.size()];
    std::copy(_audioPayload.begin(), _audioPayload.end(), out);
    return out;
}

void TSAudio::prepareFile()
{
    std::ofstream tmpfile;
    tmpfile.open("/dev/shm/audio" + _fname, std::ios::out | std::ios::binary);
    tmpfile.write((char*)this->getPayload(), this->getSize());
    tmpfile.close();
    this->_isSaved = true;
}