#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Playlist.h"
#include "TSVideo.h"
#include <vector>

extern "C"  
{ 
    #include "libavcodec/avcodec.h"  
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"  
    #include "libavdevice/avdevice.h"
    #include "libswresample/swresample.h"
    
    #include "SDL2/SDL.h"  
    #include "SDL2/SDL_thread.h"
    #include "SDL2/SDL_audio.h"
};

class Player
{
    private:
        Playlist* _playlist;
        void loadSegments();
        bool pollEvent(SDL_Event, TSVideo*);
        std::vector<TSVideo*> _tsVideo;
        size_t _currentPosition;
        SDL_Window* _playerWindow;
        SDL_Renderer* _playerRenderer;
        SDL_Texture* _playerTexture;
        AVFormatContext* _formatContext;
        int _videoStream;
        int _audioStream;
        AVFrame* _pFrame;  
        AVFrame* _pFrameYUV;
        AVCodec* _codec;
        AVCodec* _audioCodec;
        bool _paused;
        int64_t _inChLayout;
        SwrContext* _swrCtx;
        struct SwsContext* _swsCtx;
        SDL_Rect _sdlRect;
    public:
        static const uint32_t VIDEO_PID_VAL;
        static const uint32_t AUDIO_PID_VAL;
        static const uint32_t PID_MASK;
        static const uint32_t ADAPTATION_FIELD_MASK;
        static const uint32_t ADAPTATION_NO_PAYLOAD;
        static const uint32_t ADAPTATION_ONLY_PAYLOAD;
        static const uint32_t ADAPTATION_BOTH;
        Player(Playlist*);
        ~Player();
        bool playNext();
        friend void loadSegmentsThread(Player* player);
};

#endif