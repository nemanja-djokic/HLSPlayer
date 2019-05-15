#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include <vector>
#include <string>
#include <iostream>
#include "PlaylistSegment.h"

class Playlist
{
    private:
        std::vector<PlaylistSegment*>* _mediaEndpoints;
        int _targetDuration;
        int _mediaSequence;
        bool _isEnded;
        int32_t _bitrate;
    public:
        static const std::string EXT_X_TARGETDURATION_TAG;
        static const std::string EXT_X_MEDIA_SEQUENCE_TAG;
        static const std::string EXT_X_ENDLIST_TAG;
        static const std::string EXTINF_TAG;

        static Playlist* parsePlaylist(std::string, std::string);
        void* nextMediaBlock(double& duration);
        inline bool getIsEnded(){return _isEnded;};
        inline std::vector<PlaylistSegment*>* getSegments(){return _mediaEndpoints;};
        inline void setBitrate(int32_t bitrate){_bitrate = bitrate;};
        inline int32_t getBitrate(){return _bitrate;};
};

#endif