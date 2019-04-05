#include "headers/Player.h"
#include "headers/Constants.h"

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/rational.h>

    #include <SDL2/SDL.h>
    #include <SDL2/SDL_thread.h>
}

#include <iostream>
#include <fstream>
#include <ios>

const uint32_t Player::VIDEO_PID_VAL = 0x100;
const uint32_t Player::AUDIO_PID_VAL = 0x101;
const uint32_t Player::PID_MASK = 0x1fff00;

Player::Player(Playlist* playlist)
{
    this->_playlist = playlist;
    this->_currentPosition = 0;
    loadSegments();
}

void Player::loadSegments()
{
    for(unsigned int i = 0; i < (*_playlist->getSegments()).size(); i++)
    {
        (*_playlist->getSegments())[i].loadSegment();
        uint8_t* segmentPayload = (*_playlist->getSegments())[i].getTsData();
        size_t payloadSize = (*_playlist->getSegments())[i].loadedSize();
        TSVideo toInsertVideo;
        TSAudio toInsertAudio;
        for(size_t i = 0; i < payloadSize / TS_BLOCK_SIZE; i++)
        {
            uint32_t header = (*(segmentPayload + i * TS_BLOCK_SIZE + 0) << 24)
                + (*(segmentPayload + i * TS_BLOCK_SIZE + 1) << 16)
                + (*(segmentPayload + i * TS_BLOCK_SIZE + 2) << 8)
                + *(segmentPayload + i * TS_BLOCK_SIZE + 3);
            uint32_t pid = (header & PID_MASK) >> 8;
            if(pid == VIDEO_PID_VAL)
            {
                toInsertVideo.appendData(segmentPayload + (i * TS_BLOCK_SIZE + 4), TS_BLOCK_SIZE - 4);
            }
            else if (pid == AUDIO_PID_VAL)
            {
                toInsertAudio.appendData(segmentPayload + (i * TS_BLOCK_SIZE + 4), TS_BLOCK_SIZE - 4);
            }
            else
            {

            }
        }
        this->_tsVideo.push_back(toInsertVideo);
        this->_tsAudio.push_back(toInsertAudio);
    }
}

bool Player::playNext()
{
    if(_currentPosition >= this->_tsVideo.size())
        return false;
    TSVideo current = this->_tsVideo[_currentPosition++];
    uint8_t* payload = current.getPayload();
    std::ofstream outfile;
    outfile.open("0", std::ios::out | std::ios::binary | std::ios::app);
    outfile.write((char*)payload, current.getSize());
    /*AVStream* videoStreamData = nullptr;
    unsigned char* videoStreamBuffer = (unsigned char*)av_malloc(current.getSize());
    if(videoStreamBuffer == nullptr)
    {
        std::cerr << "AV_MALLOC FAIL: OUT OF MEMORY" << std::endl;
        return;
    }
    AVIOContext* ioContext = avio_alloc_context(
        videoStreamBuffer,
        current.getSize(),
        0,
        videoStreamData,
        0,
        0,
        0
    );
    AVFormatContext* formatContext = avformat_alloc_context();

    formatContext->pb = ioContext;
    formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    
    if(avformat_open_input(&formatContext, "", nullptr, nullptr) < 0)
    {
        std::cerr << "ERROR OPENING STREAM" << std::endl;
        return false;
    }

    if(avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        std::cerr << "ERROR FINDING STREAM INFORMATION" << std::endl;
        return false;
    }

    AVCodecContext *pCodecCtx = NULL;

    int videoStream=-1;
    for(unsigned int i=0; i<formatContext->nb_streams; i++)
        if(formatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) 
        {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
    {
        std::cerr << "NO VIDEO STREAM" << std::endl;
    }

    pCodecCtx=formatContext->streams[videoStream]->codec;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        std::cerr << "Could not initialize SDL - " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_Window *screen;

    screen = SDL_CreateWindow(
        "Player",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        pCodecCtx->width,
        pCodecCtx->height,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
    if(!screen) {
        std::cerr << "SDL: could not set video mode" << std::endl;
        return false;
    }*/
    return true;
}