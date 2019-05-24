#include <iostream>
#include "headers/HlsUtil.h"
#include "headers/Player.h"

int main(int argc, char* argv[])
{;
    if(argc < 2)
    {
        std::cout << "Command line arguments:" << std::endl;
        std::cout << "[HLS Stream Link (Required)] [Optional attributes]" << std::endl;
        std::cout << "Optional attributes:" << std::endl;
        std::cout << "-m [value]" << std::endl;
        std::cout << "      Sets the maximum capacity of the internal buffer to the value. Value entered is in MB. Default is 50MB." << std::endl;
        std::cout << "-w [value]" << std::endl;
        std::cout << "      Sets the pixel width of the player to the value. Default is current monitor width." << std::endl;
        std::cout << "-h [vrijednost]" << std::endl;
        std::cout << "      Sets the pixel height of the player to the value. Default is current monitor height." << std::endl;
        std::cout << "--windowed" << std::endl;
        std::cout << "      Flag attribute. If present player starts in Windowed mode. Default is full screen." << std::endl;
        std::cout << 
        "If the '--windowed' attribute is not present the resolution of the player will be equal to the current resolution" 
        << " of the monitor even if '-w' and '-h' are present." << std::endl;
        return 0;
    }
    HlsUtil util(argv[1]);
    int32_t width = -1;
    int32_t height = -1;
    int32_t maxMemory = 50;
    bool fullScreen = true;
    for(int i = 2; i < argc; i++)
    {
        if(strcmp(argv[i], "-m") == 0)
        {
            maxMemory = std::atoi(argv[i + 1]);
            if(maxMemory < 0)maxMemory = 50;
        }
        else if(strcmp(argv[i], "-w") == 0)
            width = std::atoi(argv[i + 1]);
        else if(strcmp(argv[i], "-h") == 0)
            height = std::atoi(argv[i + 1]);
        else if(strcmp(argv[i], "--windowed") == 0)
            fullScreen = false;
    }
    int res = util.readRootPlaylist();
    if(res == OK)
        std::cout << "NETWORK_OK" << std::endl;
    else if(res == CONNECTION_FAILED)
        std::cerr << "CONNECTION_FAILED" << std::endl;
    else if(res == PARSE_FAILED)
        std::cerr << "PARSE_FAILED" << std::endl;
    std::vector<int> bitrates = util.getAvailableBitrates();
    std::vector<Playlist*>* playlistVector = new std::vector<Playlist*>();
    std::vector<int32_t>* bitratesVector = new std::vector<int32_t>();
    for(std::vector<int>::iterator it = bitrates.begin(); it != bitrates.end(); ++it)
    {
        bitratesVector->insert(bitratesVector->end(), *it);
        Playlist* playlist = util.getPlaylistForBitrate(*it);
        if(playlist == nullptr)continue;
        playlist->setBitrate(*it);
        playlistVector->insert(playlistVector->end(), playlist);
    }
    std::sort(playlistVector->begin(), playlistVector->end(),
          [](Playlist* i, Playlist* j)
          {return i->getBitrate() < j->getBitrate();});
    std::sort(bitratesVector->begin(), bitratesVector->end());
    Player player(playlistVector, bitratesVector, width, height, maxMemory, fullScreen);
    int max = player.prepare();
    playlistVector->clear();
    bitratesVector->clear();
    for(std::vector<int>::iterator it = bitrates.begin(); it != bitrates.end(); ++it)
    {
        Playlist* playlist = util.getPlaylistForBitrate(*it);
        if(playlist == nullptr)continue;
        if((int32_t)playlist->getSegments()->size() != max)
        {
            continue;
        }
        playlist->setBitrate(*it);
        bitratesVector->insert(bitratesVector->end(), *it);
        playlistVector->insert(playlistVector->end(), playlist);
    }
    std::sort(playlistVector->begin(), playlistVector->end(),
          [](Playlist* i, Playlist* j)
          {return i->getBitrate() < j->getBitrate();});
    std::sort(bitratesVector->begin(), bitratesVector->end());
    player.setPlaylists(playlistVector);
    player.setBitrates(bitratesVector);
    player.loadSegments();
    player.play();
    return 0;
}