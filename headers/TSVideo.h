#ifndef _TSVIDEO_H_
#define _TSVIDEO_H_

#include <vector>
#include <cstdint>
#include <string>

class TSVideo
{
    private:
        std::vector<uint8_t> _videoPayload;
        bool _hasData;
        std::string _fname;
        bool _isSaved;
    public:
        TSVideo(std::string);
        inline bool isSaved(){return _isSaved;};
        void appendData(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _videoPayload.size();};
        uint8_t* getPayload();
        void prepareFile();
        inline std::string getFname(){return _fname;};
};

#endif