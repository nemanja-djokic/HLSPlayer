#ifndef _MASTERPLAYLIST_H_
#define _MASTERPLAYLIST_H_

#include <string>
#include <vector>
#include "ExtXMedia.h"
#include "ExtXStreamInf.h"

//Contains all possible tags that can appear in master playlist
class MasterPlaylist
{
    private:
        std::vector<ExtXMedia> _extXMedia;
        std::vector<ExtXStreamInf> _extXStreamInf;
    public:
        static MasterPlaylist* parsePlaylist(std::string);
        inline std::vector<ExtXMedia> getExtXMedia(){return _extXMedia;};
        inline std::vector<ExtXStreamInf> getExtXStreamInf(){return _extXStreamInf;};
};

#endif