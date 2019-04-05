#ifndef _TSVIDEO_H_
#define _TSVIDEO_H_

#include <vector>
#include <cstdint>

class TSVideo
{
    private:
        std::vector<uint8_t> _videoPayload;
        bool _hasData;
    public:
        void appendData(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _videoPayload.size();};
        uint8_t* getPayload();
};

#endif