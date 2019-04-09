#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Playlist.h"
#include "TSVideo.h"
#include "TSAudio.h"
#include <vector>

#include <SDL2/SDL.h>

class Player
{
    private:
        Playlist* _playlist;
        void loadSegments();
        void loadSegment(uint32_t);
        std::vector<TSVideo> _tsVideo;
        std::vector<TSAudio> _tsAudio;
        size_t _currentPosition;
        SDL_Window* _playerWindow;
        SDL_Renderer* _playerRenderer;
        SDL_Texture* _playerTexture;
    public:
        static const uint32_t VIDEO_PID_VAL;
        static const uint32_t AUDIO_PID_VAL;
        static const uint32_t PID_MASK;
        static const uint32_t ADAPTATION_FIELD_MASK;
        static const uint32_t ADAPTATION_NO_PAYLOAD;
        static const uint32_t ADAPTATION_ONLY_PAYLOAD;
        static const uint32_t ADAPTATION_BOTH;
        Player(Playlist*);
        bool playNext();
        friend void loadSegmentsThread(Player* player);
};

#endif