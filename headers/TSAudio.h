#ifndef _TSAUDIO_H_
#define _TSAUDIO_H_

#include <vector>
#include <cstdint>

class TSAudio
{
    private:
        std::vector<uint8_t> _audioPayload;
        bool _hasData;
    public:
        void appendData(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
};

#endif