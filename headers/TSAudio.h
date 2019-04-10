#ifndef _TSAUDIO_H_
#define _TSAUDIO_H_

#include <vector>
#include <cstdint>
#include <string>

class TSAudio
{
    private:
        std::vector<uint8_t> _audioPayload;
        bool _hasData;
        std::string _fname;
        bool _isSaved;
    public:
        TSAudio(std::string);
        inline bool isSaved(){return _isSaved;};
        void appendData(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _audioPayload.size();};
        uint8_t* getPayload();
        void prepareFile();
        inline std::string getFname(){return _fname;};
};

#endif