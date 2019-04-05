#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Playlist.h"
#include "TSVideo.h"
#include "TSAudio.h"
#include <vector>

class Player
{
    private:
        Playlist* _playlist;
        void loadSegments();
        std::vector<TSVideo> _tsVideo;
        std::vector<TSAudio> _tsAudio;
        size_t _currentPosition;
    public:
        static const uint32_t VIDEO_PID_VAL;
        static const uint32_t AUDIO_PID_VAL;
        static const uint32_t PID_MASK;
        Player(Playlist*);
        bool playNext();
};

#endif