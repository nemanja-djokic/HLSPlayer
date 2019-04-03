#include <iostream>
#include "headers/HlsUtil.h"

int main(int argc, char* argv[])
{
    HlsUtil util("http://localhost:8080/BigBuckBunny");
    int res = util.readRootPlaylist();
    if(res == OK)
        std::cout << "OK" << std::endl;
    else if(res == CONNECTION_FAILED)
        std::cout << "CONNECTION_FAILED" << std::endl;
    else if(res == PARSE_FAILED)
        std::cout << "PARSE_FAILED" << std::endl;
    
    return 0;
}