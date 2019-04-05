#include <iostream>
#include "headers/HlsUtil.h"
#include "headers/Player.h"

int main(int argc, char* argv[])
{
    HlsUtil util("http://localhost:8080/BigBuckBunny");
    int res = util.readRootPlaylist();
    if(res == OK)
        std::cout << "OK" << std::endl;
    else if(res == CONNECTION_FAILED)
        std::cout << "CONNECTION_FAILED" << std::endl;
    else if(res == PARSE_FAILED)
        std::cout << "PARSE_FAILED" << std::endl;
    std::vector<int> bitrates = util.getAvailableBitrates();
    int maxBitrate = 0;
    for(std::vector<int>::iterator it = bitrates.begin(); it != bitrates.end(); ++it)
    {
        maxBitrate = (maxBitrate < *it)?*it:maxBitrate;
        std::cout << *it << std::endl;
    }
    Playlist* playlist = util.getPlaylistForBitrate(maxBitrate);
    std::cout << playlist->getIsEnded() << std::endl;
    std::vector<PlaylistSegment> segments = *playlist->getSegments();
    std::cout << segments.size() << std::endl;
    Player player(playlist);
    while(player.playNext());
    delete playlist;
    return 0;
}