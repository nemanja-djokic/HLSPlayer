#include "headers/TSAudio.h"

void TSAudio::appendData(uint8_t* buffer, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        this->_audioPayload.insert(this->_audioPayload.end(), buffer[i]);
        this->_hasData = true;
    }
}