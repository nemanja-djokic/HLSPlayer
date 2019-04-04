#ifndef _MASTERPLAYLIST_H_
#define _MASTERPLAYLIST_H_

#include <string>
#include <vector>
#include "ExtXStreamInf.h"
#include "Playlist.h"

//Contains all possible tags that can appear in master playlist
class MasterPlaylist
{
    private:
        std::vector<ExtXStreamInf> _extXStreamInf;
        std::string _rootPlaylistUrl;
        std::string getEndpointForBitrate(int bitrate);
    public:
        static MasterPlaylist* parseMasterPlaylist(std::string, std::string);
        inline std::vector<ExtXStreamInf> getExtXStreamInf(){return _extXStreamInf;};
        std::vector<int> getAvailableBitrates();
        std::string getPlaylistContent(std::string);
        Playlist* getPlaylistForBitrate(int bitrate);
};

#endif