#ifndef _TSVIDEO_H_
#define _TSVIDEO_H_

extern "C"
{
    #include "libavformat/avformat.h"
    #include "SDL2/SDL_mutex.h"
}

#include "CustomIOContext.h"

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
        AVFormatContext* _formatContext;
        CustomIOContext* _ioCtx;
    public:
        TSVideo(std::string);
        inline bool isSaved(){return _isSaved;};
        void appendData(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _videoPayload.size();};
        uint8_t* getPayload();
        void prepareFile();
        inline std::string getFname(){return _fname;};
        void prepareFormatContext(AVFormatContext*);
        inline AVFormatContext* getFormatContext(){return _formatContext;};
};

#endif