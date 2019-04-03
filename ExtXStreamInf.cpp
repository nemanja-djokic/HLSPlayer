#include "headers/ExtXStreamInf.h"
#include <sstream>
#include <iostream>

const std::string ExtXStreamInf::ID_TAG = "#EXT-X-STREAM-INF:";

ExtXStreamInf::ExtXStreamInf(std::string header, std::string endpoint)
{
    this->_endpoint = endpoint;
    std::string headless = header.substr(ID_TAG.length());
    std::istringstream iss(headless);
    int lineCount = 0;
    for(std::string line; std::getline(iss, line, ','); lineCount++)
    {
        std::cout << lineCount << ": " << line.c_str() << std::endl;
    }
}