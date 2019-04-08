#include "headers/TSAudio.h"

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