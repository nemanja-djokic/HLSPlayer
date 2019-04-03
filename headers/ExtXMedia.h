#ifndef _EXTXMEDIA_H_
#define _EXTXMEDIA_H_

#include <string>

//Dummy class for now
//Media Playlist that contains alternative Renditions
class ExtXMedia
{
    private:
        std::string _type;
        std::string _uri;
        std::string _groupId;
        std::string _language;
        std::string _assocLanguage;
        std::string _default;
        std::string _autoselect;
        std::string _forced;
        std::string _instreamId;
        std::string _characteristics;
        std::string _channels;
        std::string _endpoint;
    public:
        //NOTE: This is followed by :<value>
        static const std::string ID_TAG;

        ExtXMedia(std::string, std::string);

        inline std::string getType(){return _type;};
        inline std::string getUri(){return _uri;};
        inline std::string getGroupId(){return _uri;};
        inline std::string getLanguage(){return _language;};
        inline std::string getAssocLanguage(){return _assocLanguage;};
        inline std::string getDefault(){return _default;};
        inline std::string getAutoselect(){return _autoselect;};
        inline std::string getForced(){return _forced;};
        inline std::string getInstreamId(){return _instreamId;};
        inline std::string getCharacteristics(){return _characteristics;};
        inline std::string getChannels(){return _channels;};
        inline std::string getEndpoint(){return _endpoint;};
};

#endif