#include "headers/MasterPlaylist.h"
#include "headers/ParseConstants.h"
#include <sstream>
#include <iostream>

MasterPlaylist* MasterPlaylist::parsePlaylist(std::string hlsRoot)
{
    std::istringstream iss(hlsRoot);
    int lineCount = 0;
    MasterPlaylist* toReturn = new MasterPlaylist();
    for(std::string line; std::getline(iss, line, '\r'); lineCount++)
    {
        if(line.c_str()[0] == 0xa)
            line = line.c_str() + 1; //HAX
        if(lineCount == 0 && line.compare(MASTER_PLAYLIST_FIRST_LINE) != 0)
        {
            std::cerr << "MASTER_PLAYLIST_FIRST_LINE NOT FOUND" << std::endl;
            delete toReturn;
            return nullptr;
        }
        else
        {
            try
            {
                if(line.rfind(ExtXStreamInf::ID_TAG, 0) == 0)
                {
                    std::string nextLine;
                    std::getline(iss, nextLine, '\r');
                    if(nextLine.c_str()[0] == 0xa)
                        nextLine = nextLine.c_str() + 1; //HAX
                    lineCount++;
                    toReturn->_extXStreamInf.insert(toReturn->_extXStreamInf.begin(), ExtXStreamInf(line, nextLine));
                }
                else if(line.rfind(ExtXMedia::ID_TAG, 0) == 0)
                {
                    std::string nextLine;
                    std::getline(iss, nextLine);
                    toReturn->_extXMedia.insert(toReturn->_extXMedia.begin(), ExtXMedia(line, nextLine));
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