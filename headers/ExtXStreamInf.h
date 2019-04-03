#ifndef _EXTXSTREAMINF_H_
#define _EXTXSTREAMINF_H_

#include <string>

class ExtXStreamInf
{
    private:
        std::string _bandwidth;
        std::string _averageBandwidth;
        std::string _codecs;
        std::string _resolution;
        std::string _frameRate;
        std::string _hdcpLevel;
        std::string _audio;
        std::string _video;
        std::string _subtitles;
        std::string _closedCaptions;
        std::string _endpoint;
    public:
        //NOTE: This is followed by :<value>
        static const std::string ID_TAG;

        ExtXStreamInf(std::string, std::string);

        inline std::string getBandwidth(){return _bandwidth;};
        inline std::string getAverageBandwidth(){return _averageBandwidth;};
        inline std::string getCodecs(){return _codecs;};
        inline std::string getResolution(){return _resolution;};
        inline std::string getFramerate(){return _frameRate;};
        inline std::string getHdcpLevel(){return _hdcpLevel;};
        inline std::string getAudio(){return _audio;};
        inline std::string getVideo(){return _video;};
        inline std::string getSubtitles(){return _subtitles;};
        inline std::string getClosedCaptions(){return _closedCaptions;};
        inline std::string getEndpoint(){return _endpoint;};
};

#endif