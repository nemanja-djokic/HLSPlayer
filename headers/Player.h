#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Playlist.h"
#include "TSVideo.h"
#include "NetworkManager.h"
#include <vector>
#include <string>

extern "C"  
{ 
    #include "libavcodec/avcodec.h"  
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"  
    #include "libavdevice/avdevice.h"
    #include "libswresample/swresample.h"
    #include "libavutil/time.h"
    
    #include "SDL2/SDL.h"  
    #include "SDL2/SDL_thread.h"
    #include "SDL2/SDL_audio.h"
};

class Player
{
    private:
        Playlist* _playlist;
        std::vector<Playlist*>* _playlists;
        std::vector<int32_t>* _bitrates;
        int32_t _desiredWidth;
        int32_t _desiredHeight;
        int32_t _desiredMaxMemory;
        bool _desiredFullScreen;
        bool pollEvent(SDL_Event, TSVideo*);
        TSVideo* _tsVideo;
        size_t _currentPosition;
        SDL_Window* _playerWindow;
        SDL_Renderer* _playerRenderer;
        SDL_Texture* _playerTexture;
        AVFormatContext* _formatContext;
        int _videoStream;
        int _audioStream;
        AVFrame* _pFrameYUV;
        AVCodec* _codec;
        AVCodec* _audioCodec;
        bool _paused;
        uint32_t _lastPoll;
        int64_t _inChLayout;
        SwrContext* _swrCtx;
        struct SwsContext* _swsCtx;
        SDL_Rect _sdlRect;
        std::string _statusMessage;
        int32_t _messageSetTimestamp;
    public:
        static const uint32_t VIDEO_PID_VAL;
        static const uint32_t AUDIO_PID_VAL;
        static const uint32_t PID_MASK;
        static const uint32_t ADAPTATION_FIELD_MASK;
        static const uint32_t ADAPTATION_NO_PAYLOAD;
        static const uint32_t ADAPTATION_ONLY_PAYLOAD;
        static const uint32_t ADAPTATION_BOTH;
        static const uint32_t SAMPLE_CORRECTION_PERCENT_MAX;
        Player(Playlist*, int32_t, int32_t, int32_t, bool);
        Player(std::vector<Playlist*>*, std::vector<int32_t>*, int32_t, int32_t, int32_t, bool);
        ~Player();
        void loadSegments();
        int32_t prepare();
        bool play();
        inline void setPlaylists(std::vector<Playlist*>* playlists){_playlists = playlists;};
        inline void setBitrates(std::vector<int32_t>* bitrates){_bitrates = bitrates;};
        friend void loadSegmentsThread(Player* player);
        friend void videoFunction(AVCodecContext*, SwsContext*, AVFrame*, SDL_Texture*, SDL_Rect*, SDL_Renderer*, TSVideo*,
            int32_t, int32_t, int32_t, int32_t, Player*);
};

#endif