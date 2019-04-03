#include "headers/HlsUtil.h"
#include "headers/ParseConstants.h"
#include "curl/curl.h"
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

size_t WriteCallback(void*, size_t, size_t, void*);
const int OK = 0;
const int CONNECTION_FAILED = -1;
const int PARSE_FAILED = -2;


HlsUtil::HlsUtil(std::string rootPlaylistUrl) : _isValidState(true)
{
    this->rootPlaylistUrl = rootPlaylistUrl;
}

HlsUtil::~HlsUtil()
{
    if(this->masterPlaylist != nullptr)
        delete this->masterPlaylist;
}

bool HlsUtil::isValidState()
{
    return this->_isValidState;
}

int HlsUtil::readRootPlaylist()
{
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        std::cerr << "curl_easy_init() failed" << std::endl;
        this->_isValidState = false;
        curl_easy_cleanup(curl);
        return CONNECTION_FAILED;
    }
    std::string tempBuffer;
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, this->rootPlaylistUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tempBuffer);
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        this->_isValidState = false;
        curl_easy_cleanup(curl);
        return CONNECTION_FAILED;
    }

    int parseStatus = this->parseMasterPlaylist(tempBuffer);
    
    curl_easy_cleanup(curl);
    if(parseStatus != OK)
        return PARSE_FAILED;
    return OK;
}

int HlsUtil::parseMasterPlaylist(std::string buffer)
{
    MasterPlaylist* tempPlaylist = MasterPlaylist::parsePlaylist(buffer);
    if(tempPlaylist != nullptr)
    {
        this->masterPlaylist = tempPlaylist;
        return OK;
    }
    else
    {
        return PARSE_FAILED;
    }
    
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realSize = size * nmemb;
    ((std::string*)userp)->append(static_cast<const char *>(contents), realSize);
    return realSize;
}