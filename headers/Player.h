#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "PlaylistSegment.h"
#include <vector>

class Player
{
    private:
        std::vector<PlaylistSegment> _segments;
        void loadSegments();
    public:
        Player(std::vector<PlaylistSegment>);
};

#endif