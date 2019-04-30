#include "headers/PlaylistSegment.h"
#include "headers/Constants.h"
#include "curl/curl.h"
#include <ostream>
#include <iostream>
#include <vector>
#include <cstring>

const std::string PlaylistSegment::EXTINF_TAG = "#EXTINF:";
const std::string PlaylistSegment::EXT_X_BYTERANGE_TAG = "#EXT-X-BYTERANGE:";
const std::string PlaylistSegment::EXT_X_DISCONTINUITY_TAG = "#EXT-X-DISCONTINUITY:";
const std::string PlaylistSegment::EXT_X_KEY_TAG = "#EXT-X-KEY:";
const std::string PlaylistSegment::EXT_X_MAP_TAG = "#EXT-X-MAP:";
const std::string PlaylistSegment::EXT_X_PROGRAM_DATE_TIME_TAG = "#EXT-X-PROGRAM-DATE-TIME:"; 
const std::string PlaylistSegment::EXT_X_DATERANGE_TAG = "#EXT-X-DATERANGE:";

size_t BinaryCallback(void*, size_t, size_t, void*);

PlaylistSegment::PlaylistSegment(int num, std::string header, std::string endpoint, std::string baseUrl)
{
    this->_num = num;
    this->_endpoint = endpoint;
    this->_isLoaded = false;
    this->_baseUrl = baseUrl;

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
    };
}

PlaylistSegment::~PlaylistSegment()
{
    this->unloadSegment();
}

void PlaylistSegment::loadSegment()
{
    bool loadFailed = false;
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        std::cerr << "curl_easy_init() failed" << std::endl;
        this->_isLoaded = false;
        loadFailed = true;
        curl_easy_cleanup(curl);
    }
    std::vector<uint8_t> tempBuffer;
    CURLcode res;
    int64_t lastSlash = _baseUrl.find_last_of('/');
    if(lastSlash > 0)
        _baseUrl = _baseUrl.substr(0, lastSlash + 1);
    //std::cout << "baseUrl:" << _baseUrl << std::endl;
    //std::cout << "endpoint:" << _endpoint << std::endl;
    curl_easy_setopt(curl, CURLOPT_URL, (this->_baseUrl + this->_endpoint).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tempBuffer);
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        this->_isLoaded = false;
        loadFailed = true;
        curl_easy_cleanup(curl);
    }
    if(!loadFailed)
    {
        this->_tsData = new uint8_t[tempBuffer.size()];
        this->_tsDataSize = tempBuffer.size();
        std::copy(tempBuffer.begin(), tempBuffer.end(), this->_tsData);
        //std::cout << "segment " << this->_num << " loaded " << this->_tsDataSize / TS_BLOCK_SIZE << " blocks overflow: " << this->_tsDataSize % TS_BLOCK_SIZE << std::endl;
        this->_isLoaded = true;
    }
}

void PlaylistSegment::unloadSegment()
{
    if(this->_isLoaded)
    {
        this->_isLoaded = false;
        if(_tsData)
            delete[] _tsData;
        this->_tsDataSize = 0;
    }
}

std::ostream& operator<<(std::ostream& stream, const PlaylistSegment& a)
{
    return stream << "(PlaylistSegment) "
    << a._num
    << " endpoint:" << a._endpoint
    << " extinf:" << a._extinf; 
}

size_t BinaryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realSize = size * nmemb;
    for(size_t i = 0; i < realSize; i++)
        ((std::vector<uint8_t>*)userp)->insert(((std::vector<uint8_t>*)userp)->end(), ((uint8_t*)contents)[i]);
    return realSize;
}