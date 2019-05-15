#include "headers/Playlist.h"
#include "curl/curl.h"
#include "headers/HlsUtil.h"
#include "headers/Constants.h"
#include <iostream>
#include <sstream>

const std::string Playlist::EXT_X_TARGETDURATION_TAG = "#EXT-X-TARGETDURATION:";
const std::string Playlist::EXT_X_MEDIA_SEQUENCE_TAG = "#EXT-X-TARGETDURATION:";
const std::string Playlist::EXT_X_ENDLIST_TAG = "#EXT-X-ENDLIST";
const std::string Playlist::EXTINF_TAG = "#EXTINF:";

Playlist* Playlist::parsePlaylist(std::string content, std::string baseUrl)
{
    std::istringstream iss(content);
    int lineCount = 0;
    Playlist* toReturn = new Playlist();
    toReturn->_isEnded = false;
    toReturn->_mediaEndpoints = new std::vector<PlaylistSegment*>();
    int segmentCounter = 0;
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
                std::cout << "lineCount 0" << std::endl;
            if(line.compare(MASTER_PLAYLIST_FIRST_LINE) != 0)
            {
                std::cout << line << std::endl;
                std::cout << MASTER_PLAYLIST_FIRST_LINE << std::endl;
            }
            std::cerr << "MASTER_PLAYLIST_FIRST_LINE_NOT FOUND" << std::endl;
            delete toReturn;
            return nullptr;
        }
        else if(lineCount > 0)
        {
            try
            {
                if(line.rfind(EXT_X_TARGETDURATION_TAG, 0) == 0)
                {
                    std::string targetDurationVal = line.substr(EXT_X_TARGETDURATION_TAG.length(), line.length());
                    toReturn->_targetDuration = std::stoi(targetDurationVal);
                }
                else if(line.rfind(EXT_X_MEDIA_SEQUENCE_TAG, 0) == 0)
                {
                    std::string mediaSequenceNumberStr = line.substr(EXT_X_MEDIA_SEQUENCE_TAG.length(), line.length());
                    toReturn->_mediaSequence = std::stoi(mediaSequenceNumberStr);
                }
                else if(line.rfind(EXT_X_ENDLIST_TAG, 0) == 0)
                {
                    toReturn->_isEnded = true;
                }
                else if(line.rfind(EXTINF_TAG, 0) == 0)
                {
                    std::string nextLine;
                    std::getline(iss, nextLine, '\n');
                    if(nextLine.c_str()[nextLine.length() - 1] == '\r')
                        nextLine = nextLine.substr(0, nextLine.length() - 1); //HAX
                    toReturn->_mediaEndpoints->insert(toReturn->_mediaEndpoints->end(), new PlaylistSegment(segmentCounter++, line, nextLine, baseUrl));
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