#include "headers/MasterPlaylist.h"
#include "headers/Constants.h"
#include "headers/HlsUtil.h"
#include "curl/curl.h"
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

size_t WriteCallback(void*, size_t, size_t, void*);

MasterPlaylist* MasterPlaylist::parseMasterPlaylist(std::string hlsRoot, std::string rootUrl)
{
    std::istringstream iss(hlsRoot);
    int lineCount = 0;
    MasterPlaylist* toReturn = new MasterPlaylist();
    toReturn->_rootPlaylistUrl = rootUrl;
    for(std::string line; std::getline(iss, line, '\n'); lineCount++)
    {
        line = HlsUtil::trim(line);
        if(line.empty() || line.compare("\n") == 0 || line.compare("\r\n") == 0)
            continue;
        if(line.c_str()[line.length() - 1] == '\r')
            line = line.substr(0, line.length() - 1); //HAX
        line = HlsUtil::trim(line);
        if(lineCount == 0 && line.compare(MASTER_PLAYLIST_FIRST_LINE) != 0)
        {
            if(lineCount == 0)
                std::cerr << "lineCount 0" << std::endl;
            if(line.compare(MASTER_PLAYLIST_FIRST_LINE) != 0)
            {
                std::cout << "comparing:" << std::endl;
                std::cout << line << std::endl;
                std::cout << MASTER_PLAYLIST_FIRST_LINE << std::endl;
            }
            std::cerr << "MASTER_PLAYLIST_FIRST_LINE_NOT_FOUND" << std::endl;
            delete toReturn;
            return nullptr;
        }
        else
        {
            try
            {
                line = HlsUtil::trim(line);
                if(line.rfind(ExtXStreamInf::ID_TAG, 0) == 0)
                {
                    std::string nextLine;
                    std::getline(iss, nextLine, '\n');
                    if(line.c_str()[line.length() - 1] == '\r')
                        line = line.substr(0, line.length() - 1); //HAX
                    lineCount++;
                    toReturn->_extXStreamInf.insert(toReturn->_extXStreamInf.begin(), ExtXStreamInf(line, nextLine));
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                delete toReturn;
                return nullptr;
            }
            
        }
    }
    return toReturn;
}

std::vector<int> MasterPlaylist::getAvailableBitrates()
{
    std::vector<int> toReturn;
    for(std::vector<ExtXStreamInf>::iterator it = _extXStreamInf.begin(); it != _extXStreamInf.end(); ++it)
    {
        std::cout << *it << std::endl;
        toReturn.insert(toReturn.begin(), (int)std::stoi(it->getBandwidth()));
    }
    return toReturn;
}

std::string MasterPlaylist::getEndpointForBitrate(int bitrate)
{
    for(std::vector<ExtXStreamInf>::iterator it = _extXStreamInf.begin(); it != _extXStreamInf.end(); ++it)
    {
        int iterBitrate = std::stof(it->getBandwidth());
        if(bitrate == iterBitrate)
            return it->getEndpoint();
    }
    return nullptr;
}

std::string MasterPlaylist::getPlaylistContent(std::string endpoint)
{
    if(endpoint.c_str()[endpoint.length() - 1] == '\n')
            endpoint = endpoint.substr(0, endpoint.length() - 1); //HAX

    if(endpoint.c_str()[endpoint.length() - 1] == '\r')
            endpoint = endpoint.substr(0, endpoint.length() - 1); //HAX
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        std::cerr << "curl_easy_init() failed" << std::endl;
        curl_easy_cleanup(curl);
        return nullptr;
    }
    std::string tempBuffer;
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tempBuffer);
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return nullptr;
    }

    return tempBuffer;
}

bool hasEnding (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

Playlist* MasterPlaylist::getPlaylistForBitrate(int bitrate)
{
    if(hasEnding(_rootPlaylistUrl, ".m3u8"))
    {
        int lastSlash = _rootPlaylistUrl.find_last_of('/');
        _rootPlaylistUrl = _rootPlaylistUrl.substr(0, lastSlash);
    }
    if(this->_rootPlaylistUrl[_rootPlaylistUrl.length() - 1] != '/')
        _rootPlaylistUrl = _rootPlaylistUrl + "/";
    for(std::vector<ExtXStreamInf>::iterator it = this->_extXStreamInf.begin(); it != this->_extXStreamInf.end(); ++it)
    {
        if(std::stoi(it->getBandwidth()) == bitrate)
        {
            std::string endpointToPass = _rootPlaylistUrl;
            if(it->getEndpoint().rfind("http", 0) == 0)
            {
                endpointToPass = it->getEndpoint();
            }
            else
            {
                int64_t slashPos = it->getEndpoint().find_last_of('/');
                if(slashPos != -1)
                {
                    endpointToPass += it->getEndpoint().substr(0, slashPos + 1);
                }
            }
            std::cout << "rootPlaylistUrl:" << _rootPlaylistUrl << std::endl;
            std::cout << "itEndpoint:" << it->getEndpoint() << std::endl;
            std::cout << "endpointToPass:" << endpointToPass << std::endl;
            return Playlist::parsePlaylist(getPlaylistContent(
                ((it->getEndpoint().rfind("http", 0) == 0)?(it->getEndpoint()):(this->_rootPlaylistUrl + it->getEndpoint()))),
                endpointToPass
                );
        }
    }
    return nullptr;
}