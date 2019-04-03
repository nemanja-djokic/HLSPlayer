#ifndef _HLSUTIL_H_
#define _HLSUTIL_H_

#include <string>
#include "MasterPlaylist.h"

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
        int testConnection();
        int readRootPlaylist();

};

#endif