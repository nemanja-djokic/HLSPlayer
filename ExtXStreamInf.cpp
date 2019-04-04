#include "headers/ExtXStreamInf.h"
#include <sstream>
#include <iostream>

const std::string ExtXStreamInf::ID_TAG = "#EXT-X-STREAM-INF:";

const std::string ExtXStreamInf::PROGRAM_ID_TAG = "PROGRAM-ID";
const std::string ExtXStreamInf::BANDWIDTH_TAG = "BANDWIDTH";
const std::string ExtXStreamInf::AVERAGE_BANDWIDTH_TAG = "AVERAGE-BANDWIDTH";
const std::string ExtXStreamInf::CODECS_TAG = "CODECS";
const std::string ExtXStreamInf::RESOLUTION_TAG = "RESOLUTION";
const std::string ExtXStreamInf::FRAME_RATE_TAG = "FRAME-RATE";
const std::string ExtXStreamInf::HDCP_LEVEL_TAG = "HDCP-LEVEL";
const std::string ExtXStreamInf::AUDIO_TAG = "AUDIO";
const std::string ExtXStreamInf::VIDEO_TAG = "VIDEO";
const std::string ExtXStreamInf::SUBTITLES_TAG = "SUBTITLES";
const std::string ExtXStreamInf::CLOSED_CAPTIONS_TAG = "CLOSED-CAPTIONS";

ExtXStreamInf::ExtXStreamInf(std::string header, std::string endpoint)
{
    this->_endpoint = endpoint;
    std::string headless = header.substr(ID_TAG.length());
    std::istringstream iss(headless);
    int lineCount = 0;
    for(std::string line; std::getline(iss, line, ','); lineCount++)
    {
        std::istringstream attributeStream(line);
        for(std::string attributeName; std::getline(attributeStream, attributeName, '=');)
        {   
            if(attributeName.compare(PROGRAM_ID_TAG) == 0)
                std::getline(attributeStream, this->_programId, '=');
            else if(attributeName.compare(BANDWIDTH_TAG) == 0)
                std::getline(attributeStream, this->_bandwidth, '=');
            else if(attributeName.compare(AVERAGE_BANDWIDTH_TAG) == 0)
                std::getline(attributeStream, this->_averageBandwidth, '=');
            else if(attributeName.compare(CODECS_TAG) == 0)
                std::getline(attributeStream, this->_codecs, '=');
            else if(attributeName.compare(RESOLUTION_TAG) == 0)
                std::getline(attributeStream, this->_resolution, '=');
            else if(attributeName.compare(FRAME_RATE_TAG) == 0)
                std::getline(attributeStream, this->_frameRate, '=');
            else if(attributeName.compare(HDCP_LEVEL_TAG) == 0)
                std::getline(attributeStream, this->_hdcpLevel, '=');
            else if(attributeName.compare(AUDIO_TAG) == 0)
                std::getline(attributeStream, this->_audio, '=');
            else if(attributeName.compare(VIDEO_TAG) == 0)
                std::getline(attributeStream, this->_video, '=');
            else if(attributeName.compare(SUBTITLES_TAG) == 0)
                std::getline(attributeStream, this->_subtitles, '=');
            else if(attributeName.compare(CLOSED_CAPTIONS_TAG) == 0)
                std::getline(attributeStream, this->_closedCaptions, '=');
        }
    }
}

std::ostream& operator<<(std::ostream& stream, const ExtXStreamInf& a)
{
    return stream << "(ExtXStreamInf) " 
    << ((!a._programId.empty())?" program-id:":"") << ((!a._programId.empty())?a._programId:"")
    << ((!a._bandwidth.empty())?" bandwidth:":"") << ((!a._bandwidth.empty())?a._bandwidth:"")
    << ((!a._averageBandwidth.empty())?" average-bandwidth:":"") << ((!a._averageBandwidth.empty())?a._averageBandwidth:"")
    << ((!a._codecs.empty())?" codecs:":"") << ((!a._codecs.empty())?a._codecs:"")
    << ((!a._resolution.empty())?" resolution:":"") << ((!a._resolution.empty())?a._resolution:"")
    << ((!a._frameRate.empty())?" frame-rate:":"") << ((!a._frameRate.empty())?a._frameRate:"")
    << ((!a._hdcpLevel.empty())?" hdcp-level:":"") << ((!a._hdcpLevel.empty())?a._hdcpLevel:"")
    << ((!a._audio.empty())?" audio:":"") << ((!a._audio.empty())?a._audio:"")
    << ((!a._video.empty())?" video:":"") << ((!a._video.empty())?a._video:"")
    << ((!a._subtitles.empty())?" subtitles:":"") << ((!a._subtitles.empty())?a._subtitles:"")
    << ((!a._closedCaptions.empty())?" closed-captions:":"") << ((!a._closedCaptions.empty())?a._closedCaptions:"")
    << " endpoint:" << a._endpoint;
}