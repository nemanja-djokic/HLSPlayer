#ifndef _HLSUTIL_H_
#define _HLSUTIL_H_

#include <string>
#include <algorithm>
#include "MasterPlaylist.h"
#include "Playlist.h"

extern const int OK;
extern const int CONNECTION_FAILED;
extern const int PARSE_FAILED;

class HlsUtil
{
    private:
        bool _isValidState;
        std::string rootPlaylistUrl;
        MasterPlaylist* masterPlaylist;
        int parseMasterPlaylist(std::string);
    public:
        HlsUtil(std::string);
        ~HlsUtil();
        bool isValidState();
        int readRootPlaylist();
        static inline std::string &ltrim(std::string &s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
            return s;
        }
        static inline std::string &rtrim(std::string &s)
        {
            s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
            return s;
        }
        static inline std::string &trim(std::string &s)
        {
            return ltrim(rtrim(s));
        }
        inline std::vector<int> getAvailableBitrates(){return masterPlaylist->getAvailableBitrates();};
        Playlist* getPlaylistForBitrate(int bitrate);
};

#endif