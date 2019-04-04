#include "headers/PlaylistSegment.h"
#include <ostream>
#include <iostream>

const std::string PlaylistSegment::EXTINF_TAG = "#EXTINF:";
const std::string PlaylistSegment::EXT_X_BYTERANGE_TAG = "#EXT-X-BYTERANGE:";
const std::string PlaylistSegment::EXT_X_DISCONTINUITY_TAG = "#EXT-X-DISCONTINUITY:";
const std::string PlaylistSegment::EXT_X_KEY_TAG = "#EXT-X-KEY:";
const std::string PlaylistSegment::EXT_X_MAP_TAG = "#EXT-X-MAP:";
const std::string PlaylistSegment::EXT_X_PROGRAM_DATE_TIME_TAG = "#EXT-X-PROGRAM-DATE-TIME:"; 
const std::string PlaylistSegment::EXT_X_DATERANGE_TAG = "#EXT-X-DATERANGE:";

PlaylistSegment::PlaylistSegment(int num, std::string header, std::string endpoint, std::string baseUrl)
{

    this->_num = num;
    this->_endpoint = endpoint;
    std::cout << "ENDPOINT:" << endpoint << std::endl;

    if(header.rfind(EXTINF_TAG, 0) == 0)
    {
        std::string val = header.substr(EXTINF_TAG.length(), header.length());
        this->_extinf = std::stod(val);
    }
    else if(header.rfind(EXT_X_BYTERANGE_TAG, 0) == 0)
    {
        std::string val = header.substr(EXT_X_BYTERANGE_TAG.length(), header.length());
        int atPos = val.rfind("@", 0);
        if(atPos > -1)
        {
            this->_extXByterangeN = stoi(val.substr(0, atPos));
            this->_extXByterangeO = stoi(val.substr(atPos + 1));
        }
        else
        {
            this->_extXByterangeN = std::stoi(val);
            this->_extXByterangeO = -1;
        }
        
    }
    else if(header.rfind(EXT_X_DISCONTINUITY_TAG, 0) == 0)
    {
        this->_isDiscontinuity = true;
    }
    else if(header.rfind(EXT_X_KEY_TAG, 0) == 0)
    {
        std::string val = header.substr(EXT_X_KEY_TAG.length(), header.length());
        this->_extXKey = val;
    }
    else if(header.rfind(EXT_X_PROGRAM_DATE_TIME_TAG, 0) == 0)
    {
        std::string val = header.substr(EXT_X_PROGRAM_DATE_TIME_TAG.length(), header.length());
        this->_extXProgramDateTime = val;
    }
    else if(header.rfind(EXT_X_DATERANGE_TAG, 0) == 0)
    {
        std::string val = header.substr(EXT_X_DATERANGE_TAG.length(), header.length());
        this->_extXDateRange = val;
    }
}

std::ostream& operator<<(std::ostream& stream, const PlaylistSegment& a)
{
    return stream << "(PlaylistSegment) "
    << a._num
    << " endpoint:" << a._endpoint
    << " extinf:" << a._extinf; 
}