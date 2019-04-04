#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include <vector>
#include <string>
#include "PlaylistSegment.h"

class Playlist
{
    private:
        std::vector<PlaylistSegment> _mediaEndpoints;
        int _targetDuration;
        int _mediaSequence;
        bool _isEnded;
    public:
        static const std::string EXT_X_TARGETDURATION_TAG;
        static const std::string EXT_X_MEDIA_SEQUENCE_TAG;
        static const std::string EXT_X_ENDLIST_TAG;

        //Takes fully read content of the playlist endpoint
        static Playlist* parsePlaylist(std::string, std::string);
        //Returns data, duration is out param, filled with duration of the segment
        void* nextMediaBlock(double& duration);
        inline bool getIsEnded(){return _isEnded;};
        inline std::vector<PlaylistSegment> getSegments(){return _mediaEndpoints;};
};

#endif