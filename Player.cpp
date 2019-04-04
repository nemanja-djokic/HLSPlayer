#include "headers/Player.h"
#include <iostream>

Player::Player(std::vector<PlaylistSegment> segments)
{
    this->_segments = segments;
    loadSegments();
}

void Player::loadSegments()
{
    size_t averageSize = 0;
    for(unsigned int i = 0; i < _segments.size(); i++)
    {
        _segments[i].loadSegment();
        averageSize += _segments[i].loadedSize();
    }
}