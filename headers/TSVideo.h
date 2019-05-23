#ifndef _TSVIDEO_H_
#define _TSVIDEO_H_

extern "C"
{
    #include "libavformat/avformat.h"
    #include "SDL2/SDL_mutex.h"
    #include "SDL2/SDL_ttf.h"
}

#include "PlaylistSegment.h"
#include "CustomIOContext.h"

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <queue>

class TSVideo
{
    private:
        std::vector<uint8_t> _videoPayload;
        bool _hasData;
        bool _isSaved;
        bool _acceptsInterrupts;
        volatile bool _audioThreadRunning;
        bool _enqueuedFirstAudio;
        int32_t _framesToSkip;
        int32_t _underMsDelay;
        uint32_t _lastPoll;
        int32_t _currentReferencePts;
        std::vector<int32_t>* _tsBlockBegin;
        std::vector<double>* _tsBlockDuration;
        std::vector<int32_t>* _tsBlockSize;
        uint32_t _blockBufferSize;
        SDL_mutex* _videoPlayerMutex;
        SDL_mutex* _videoQueueMutex;
        SDL_mutex* _audioQueueMutex;
        SDL_semaphore* _audioSemaphore;
        double _differenceCumulative;
        int32_t _differenceCounter;
        TTF_Font* _font;
        std::queue<AVPacket>* _videoQueue;
        std::queue<AVPacket>* _audioQueue;
        uint32_t _lastTimestamp;
        AVCodecContext* _videoCodec;
        AVCodecContext* _audioCodec;
        AVFormatContext* _formatContext;
        CustomIOContext* _ioCtx;
    public:
        TSVideo();
        ~TSVideo();
        friend class Player;
        friend class CustomIOContext;
        uint32_t getFullDuration();
        inline uint8_t* getAudioChunk(){return _ioCtx->_audioChunk;};
        inline uint8_t* getAudioPos(){return _ioCtx->_audioPos;};
        inline uint32_t getAudioLen(){return _ioCtx->_audioLen;};
        inline int32_t getFramesToSkip(){return _framesToSkip;};
        inline bool getAudioThreadRunning(){return _audioThreadRunning;};
        inline void setAudioThreadRunning(bool audioThreadRunning){_audioThreadRunning = audioThreadRunning;};
        inline bool getEnqueuedFirstAudio(){return _enqueuedFirstAudio;};
        inline void setEnqueuedFirstAudio(bool enqueuedFirstAudio){_enqueuedFirstAudio = enqueuedFirstAudio;};
        inline void setFramesToSkip(int32_t framesToSkip){_framesToSkip = framesToSkip;};
        inline SDL_semaphore* getAudioSemaphore(){return _audioSemaphore;};
        inline void setAudioChunk(uint8_t* audioChunk){_ioCtx->_audioChunk = audioChunk;};
        inline void setAudioPos(uint8_t* audioPos){_ioCtx->_audioPos = audioPos;};
        inline void setAudioLen(uint32_t audioLen){_ioCtx->_audioLen = audioLen;};
        inline SDL_mutex* getAudioMutex(){return _ioCtx->_audioMutex;};
        inline uint32_t getLastTimestamp(){return _lastTimestamp;};
        inline void setLastTimestamp(uint32_t timestamp){_lastTimestamp = timestamp;};
        inline bool isSaved(){return _isSaved;};
        inline int32_t getUnderMsDelay(){return _underMsDelay;};
        inline void setUnderMsDelay(int32_t newDelay){_underMsDelay = newDelay;};
        inline TTF_Font* getFont(){return _font;};
        inline bool getHasData(){return _hasData;};
        inline size_t getSize(){return _videoPayload.size();};
        void prepareFormatContext();
        inline AVFormatContext* getFormatContext(){return _formatContext;};
        void seek(int64_t, int64_t, int64_t);
        inline int32_t getCurrentPlayingSegment(){return _ioCtx->_block;};
        inline bool isResetAudio(){return _ioCtx->isResetAudio();};
        inline void setVolumeRate(int32_t volumeRate){this->_ioCtx->_volumeRate = volumeRate;};
        inline int32_t getVolumeRate(){return this->_ioCtx->_volumeRate;};
        inline bool isSoftResetAudio(){return _ioCtx->_softResetAudio;};
        inline void clearResetAudio(){_ioCtx->clearResetAudio();};
        inline void clearSoftResetAudio(){_ioCtx->_softResetAudio = false;};
        inline bool isLastBlock(){return _ioCtx->_lastBlock;};
        inline bool isBuffersSafe(){return _videoQueue->size() > 0 && _audioQueue->size() > 0;};
        inline SDL_mutex* getVideoPlayerMutex(){return _videoPlayerMutex;};
        inline CustomIOContext* getCustomIOContext(){return _ioCtx;};
        inline void assignVideoCodec(AVCodecContext* videoCodec){_videoCodec = videoCodec;_ioCtx->_videoCodec = videoCodec;};
        inline void assignAudioCodec(AVCodecContext* audioCodec){_audioCodec = audioCodec;_ioCtx->_audioCodec = audioCodec;};
        inline int32_t getAudioQueueSize(){return _audioQueue->size();};
        inline int32_t getVideoQueueSize(){return _videoQueue->size();};
        int32_t setManualBitrate(int32_t);
        void setAutomaticBitrate();
        uint32_t getSeconds();
        inline void setReferencePts(int32_t referencePts){_currentReferencePts = referencePts;};
        inline int32_t getReferencePts(){return _currentReferencePts;};
        inline void enqueueVideo(AVPacket packet)
        {
            SDL_LockMutex(_videoQueueMutex);
            _videoQueue->push(packet);
            SDL_UnlockMutex(_videoQueueMutex);
        };
        inline AVPacket dequeueVideo()
        {
            SDL_LockMutex(_videoQueueMutex);
            if(_videoQueue->empty())
            {
                SDL_UnlockMutex(_videoQueueMutex);
                AVPacket toReturn;
                toReturn.data = nullptr;
                return toReturn;
            }
            if(_videoQueue->size() == 0)
            {
                SDL_UnlockMutex(_videoQueueMutex);
                AVPacket toReturn;
                toReturn.data = nullptr;
                return toReturn;
            }
            AVPacket toRet = _videoQueue->front();
            _videoQueue->pop();
            SDL_UnlockMutex(_videoQueueMutex);
            return toRet;
        }

        inline void enqueueAudio(AVPacket packet)
        {
            SDL_LockMutex(_audioQueueMutex);
            _audioQueue->push(packet);
            SDL_UnlockMutex(_audioQueueMutex);
        };
        inline AVPacket dequeueAudio()
        {
            SDL_LockMutex(_audioQueueMutex);
            if(_audioQueue->empty())
            {
                SDL_UnlockMutex(_audioQueueMutex);
                AVPacket toReturn;
                toReturn.data = nullptr;
                return toReturn;
            }
            if(_audioQueue->size() == 0)
            {
                SDL_UnlockMutex(_audioQueueMutex);
                AVPacket toReturn;
                toReturn.data = nullptr;
                return toReturn;
            }
            AVPacket toRet = _audioQueue->front();
            _audioQueue->pop();
            SDL_UnlockMutex(_audioQueueMutex);
            return toRet;
        };
};
#endif