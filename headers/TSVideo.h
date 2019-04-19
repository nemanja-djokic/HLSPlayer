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
#include <iostream>

class TSVideo
{
    private:
        std::vector<uint8_t> _videoPayload;
        bool _hasData;
        std::string _fname;
        bool _isSaved;
        double _currentPts;
        int64_t _currentPtsTime;
        std::vector<int32_t>* _tsBlockBegin;
        std::vector<double>* _tsBlockDuration;
        std::vector<int32_t>* _tsBlockSize;
        uint32_t _blockBufferSize;
        SDL_mutex* _videoPlayerMutex;
        double _differenceCumulative;
        int32_t _differenceCounter;
        AVCodecContext* _videoCodec;
        AVCodecContext* _audioCodec;
        AVFormatContext* _formatContext;
        CustomIOContext* _ioCtx;
    public:
        TSVideo(std::string);
        ~TSVideo();
        inline bool isSaved(){return _isSaved;};
        void appendData(uint8_t*, size_t, bool, bool, double);
        void appendAudio(uint8_t*, size_t);
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _videoPayload.size();};
        uint8_t* getPayload();
        void prepareFile();
        inline std::string getFname(){return _fname;};
        void prepareFormatContext(AVFormatContext*);
        inline AVFormatContext* getFormatContext(){return _formatContext;};
        void seek(int64_t, int64_t);
        inline bool isResetAudio(){return _ioCtx->_resetAudio;};
        inline void clearResetAudio(){_ioCtx->_resetAudio = false;};
        inline double getCurrentPTS(){return _currentPts;};
        inline int64_t getCurrentPTSTime(){return _currentPtsTime;};
        void refreshTimer(AVPacket);
        double getVideoClock();
        double getExternalClock();
        double synchronizeVideo(AVFrame*, double);
        int32_t synchronizeAudio(uint32_t, double);
        void sizeAccumulate();
        inline SDL_mutex* getVideoPlayerMutex(){return _videoPlayerMutex;};
        inline CustomIOContext* getCustomIOContext(){return _ioCtx;};
        inline void assignVideoCodec(AVCodecContext* videoCodec){_videoCodec = videoCodec;};
        inline void assignAudioCodec(AVCodecContext* audioCodec){_audioCodec = audioCodec;};
        inline void setVideoPts(double videoPts){_ioCtx->_videoPts = videoPts;};
        inline void setAudioPts(double audioPts){_ioCtx->_audioPts = audioPts;};
};

#endif