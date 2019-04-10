#include "headers/Player.h"
#include "headers/Constants.h"

extern "C"  
{ 
    #include "libavcodec/avcodec.h"  
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"  
    #include "libavdevice/avdevice.h"
    #include "libswresample/swresample.h"
    
    #include "SDL2/SDL.h"  
    #include "SDL2/SDL_thread.h"
    #include "SDL2/SDL_audio.h"
};
#include <iostream>
#include <fstream>
#include <ios>
#include <thread>
#include <cassert>

void loadSegmentsThread(Player* player)
{
    player->loadSegments();
}

const uint32_t Player::VIDEO_PID_VAL = 0x100;
const uint32_t Player::AUDIO_PID_VAL = 0x101;
const uint32_t Player::PID_MASK = 0x1fff00;
const uint32_t Player::ADAPTATION_FIELD_MASK = 0x30;
const uint32_t Player::ADAPTATION_NO_PAYLOAD = 0b10;
const uint32_t Player::ADAPTATION_ONLY_PAYLOAD = 0b01;
const uint32_t Player::ADAPTATION_BOTH = 0b11;

Player::Player(Playlist* playlist)
{
    this->_playlist = playlist;
    this->_currentPosition = 0;
    this->_playerWindow = nullptr;
    this->_playerRenderer = nullptr;
    av_register_all();
	avcodec_register_all();
	avdevice_register_all();
    avformat_network_init();
    std::thread(loadSegmentsThread, this).detach();
    //av_log_set_level(AV_LOG_PANIC);
}

void Player::loadSegments()
{
    for(unsigned int i = 0; i < (*_playlist->getSegments()).size(); i++)
    {
        (*_playlist->getSegments())[i].loadSegment();
        uint8_t* segmentPayload = (*_playlist->getSegments())[i].getTsData();
        size_t payloadSize = (*_playlist->getSegments())[i].loadedSize();
        TSVideo toInsertVideo((*_playlist->getSegments())[i].getEndpoint());
        for(size_t i = 0; i < payloadSize / TS_BLOCK_SIZE; i++)
        {
            toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
        }
        toInsertVideo.prepareFile();
        this->_tsVideo.push_back(toInsertVideo);
    }
}

void Player::loadSegment(uint32_t pos)
{
    (*_playlist->getSegments())[pos].loadSegment();
    uint8_t* segmentPayload = (*_playlist->getSegments())[pos].getTsData();
    size_t payloadSize = (*_playlist->getSegments())[pos].loadedSize();
    TSVideo toInsertVideo((*_playlist->getSegments())[pos].getEndpoint());
    for(size_t i = 0; i < payloadSize / TS_BLOCK_SIZE; i++)
    {
        toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
    }
    toInsertVideo.prepareFile();
    this->_tsVideo.push_back(toInsertVideo);
}

static uint8_t *audio_chunk; 
static uint32_t audio_len; 
static uint8_t *audio_pos;
bool gotFirstVideo = false;
bool gotFirstAudio = false;

void  fill_audio(void *udata, Uint8 *stream, int len){
	if(gotFirstVideo && gotFirstAudio)
    {
        SDL_memset(stream, 0, len);
	    if(audio_len==0)
		    return; 

	    len=((uint32_t)len>audio_len?audio_len:len);

	    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
	    audio_pos += len; 
	    audio_len -= len; 
    }
}

bool Player::playNext()
{
    while(!(*this->_playlist->getSegments())[_currentPosition].getIsLoaded());
    while(_currentPosition >= this->_tsVideo.size());
    TSVideo current = this->_tsVideo[_currentPosition];
    _currentPosition++;
    AVFormatContext *formatContext = nullptr;
    if(avformat_open_input(&formatContext, (std::string("/dev/shm/") + current.getFname()).c_str(), nullptr, nullptr) != 0)
    {
        std::cerr << "Format context failed" << std::endl;
        return false;
    }
    if(avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        std::cerr << "Stream info failed" << std::endl;
        return false;
    }

    av_dump_format(formatContext, 0, "/dev/shm/tmp", 0);

    int videoStream = -1;
    int audioStream = -1;
    for(unsigned int i=0; i < formatContext->nb_streams; i++) {  
        if( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0) {  
            videoStream = i;
        }
        if(formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0)
        {
            audioStream = i;
        }
    }  
    if(videoStream == -1 || audioStream == -1) {  
        std::cerr << "Failed finding stream" << std::endl;
        return false;
    }

    AVCodecContext* codecContext = formatContext->streams[videoStream]->codec;
    AVCodecContext* audioCodecContextOrig = formatContext->streams[audioStream]->codec;
    AVCodecContext* audioCodecContext;

    AVCodec* codec = avcodec_find_decoder(codecContext->codec_id);
    AVCodec* audioCodec = avcodec_find_decoder(audioCodecContextOrig->codec_id);
    if(codec == nullptr || audioCodec == nullptr) {  
        std::cerr << "Unsupported codec" << std::endl;  
        return false;
    }
    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec audioSpec;

    if(current.getFname() == _tsVideo.at(0).getFname())
    {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
        {
            std::cerr << "Could not initialize SDL - " << SDL_GetError() << std::endl;
            return false;
        }
        audioCodecContext = avcodec_alloc_context3(audioCodec);

        if(avcodec_copy_context(audioCodecContext, audioCodecContextOrig) != 0)
        {
            std::cerr << "Couldn't copy audio codec context" << std::endl;
            return false;
        }
        wantedSpec.freq = audioCodecContext->sample_rate;
        wantedSpec.format = AUDIO_S16SYS;
        wantedSpec.channels = audioCodecContext->channels;
        wantedSpec.silence = 0;
        wantedSpec.samples = 1024;
        wantedSpec.callback = fill_audio;
        wantedSpec.userdata = audioCodecContext;
        std::cout << "CHANNELS:" << wantedSpec.channels << std::endl;
        if(SDL_OpenAudio(&wantedSpec, &audioSpec) < 0)
        {
            std::cerr << "SDL_OpenAudio " << SDL_GetError() << std::endl;
            return false;
        }
    }

    int64_t inChLayout = av_get_default_channel_layout(audioCodecContext->channels);
    SwrContext* swrCtx = swr_alloc();
    swrCtx = swr_alloc_set_opts(swrCtx, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, audioCodecContext->sample_rate, inChLayout, audioCodecContext->sample_fmt,
        audioCodecContext->sample_rate, 0, nullptr);
    swr_init(swrCtx);

    SDL_PauseAudio(0);
    AVDictionary* optionsDictionary = nullptr;
    if(avcodec_open2(codecContext, codec, &optionsDictionary) < 0 || avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0)
    {  
        std::cerr << "Could not open codec" << std::endl;
        return false; 
    }
	AVPixelFormat dst_fix_fmt = AV_PIX_FMT_BGR24;
    AVFrame* pFrame = av_frame_alloc();  
    AVFrame* pFrameYUV = av_frame_alloc();
    if(pFrameYUV == nullptr) {
        std::cerr << "Bad frame format" << std::endl;
        return false;  
    }

    struct SwsContext* sws_ctx = sws_getContext(
        codecContext->width,  
        codecContext->height,  
        codecContext->pix_fmt,  
        codecContext->width,  
        codecContext->height,  
        dst_fix_fmt,  
        SWS_BILINEAR,  
        NULL,  
        NULL,  
        NULL);  
  
    int numBytes = avpicture_get_size(  
        dst_fix_fmt,  
        codecContext->width,  
        codecContext->height);  
    uint8_t* buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));  
  
    avpicture_fill((AVPicture *)pFrameYUV, buffer, dst_fix_fmt,  
        codecContext->width, codecContext->height);
    SDL_Rect sdlRect;  
    sdlRect.x = 0;  
    sdlRect.y = 0;  
    sdlRect.w = codecContext->width;  
    sdlRect.h = codecContext->height;
    if(_playerWindow == nullptr)
    {
        _playerWindow = SDL_CreateWindow("HLSPlayer",  
        SDL_WINDOWPOS_UNDEFINED,  
        SDL_WINDOWPOS_UNDEFINED,  
        codecContext->width,  codecContext->height,  
        0);  
        if(!_playerWindow)
        {
            std::cerr << "SDL: could not set video mode" << std::endl;
            return false;
        }
        _playerRenderer = SDL_CreateRenderer(_playerWindow, -1, SDL_RENDERER_TARGETTEXTURE);

        _playerTexture = SDL_CreateTexture(  
            _playerRenderer,  
            SDL_PIXELFORMAT_BGR24,  
            SDL_TEXTUREACCESS_STATIC, 
            codecContext->width,  
            codecContext->height);  
	    if(!_playerTexture)
		    return false;
	    SDL_SetTextureBlendMode(_playerTexture,SDL_BLENDMODE_BLEND);
    }
    AVPacket packet;  
    SDL_Event event;

    int gotPicture;
    uint32_t videoDelay = 0;
    while(av_read_frame(formatContext, &packet) >= 0)
    {
        if(packet.stream_index == videoStream)
        {
            uint32_t startTicks = SDL_GetTicks();
            int frameFinished;
            avcodec_decode_video2(codecContext, pFrame, &frameFinished, &packet);
            if(frameFinished)
            {
                gotFirstVideo = true;
                if(gotFirstVideo && gotFirstAudio)
                {
                    sws_scale(  
                    sws_ctx,  
                    (uint8_t const * const *)pFrame->data,  
                    pFrame->linesize,  
                    0,  
                    codecContext->height,  
                    pFrameYUV->data,
                    pFrameYUV->linesize  
                    );  
                  
                    SDL_UpdateTexture(_playerTexture, &sdlRect, pFrameYUV->data[0], pFrameYUV->linesize[0]);  
                    SDL_RenderClear(_playerRenderer);  
                    SDL_RenderCopy(_playerRenderer, _playerTexture, &sdlRect, &sdlRect);  
                    SDL_RenderPresent(_playerRenderer);
                }
            }
            int framerate = av_q2d(codecContext->framerate);
            uint32_t endTicks = SDL_GetTicks();
            videoDelay = (uint32_t)(1000/(double)framerate) - (endTicks - startTicks);
            SDL_Delay(videoDelay);
        }
        else if(packet.stream_index == audioStream && gotFirstVideo)
        {
            int ret = avcodec_decode_audio4(audioCodecContext, pFrame, &gotPicture, &packet);
            if(ret < 0)
            {
                std::cerr << "Error decoding audio" << std::endl;
                return false;
            }
            int outBufferSize = av_samples_get_buffer_size(NULL,1 , audioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);
            uint8_t* outBuffer=(uint8_t *)av_malloc(192000 * 2);
            if(gotPicture > 0)
            {
                swr_convert(swrCtx, &outBuffer, 192000, (const uint8_t **)pFrame->data, pFrame->nb_samples);
            }
            uint8_t* tempBuffer = (uint8_t*)av_malloc(audio_len + outBufferSize);
            memcpy(tempBuffer, audio_pos, audio_len);
            memcpy(tempBuffer + audio_len, outBuffer, outBufferSize);
            av_free(audio_chunk);
            audio_chunk = (uint8_t*)tempBuffer;
			audio_len = audio_len + outBufferSize;
            audio_pos = audio_chunk;
            gotFirstAudio = true;
        }
        av_free_packet(&packet);  
        SDL_PollEvent(&event);  
        switch( event.type )
        {  
            case SDL_QUIT:  
                SDL_Quit();  
                return false;
                break;  
            default:  
                break;  
        }
    }
    swr_free(&swrCtx);
    av_free(pFrame);  
    av_free(pFrameYUV);   
    avcodec_close(codecContext);  
    avformat_close_input(&formatContext);
    gotFirstVideo = false;
    gotFirstAudio = false;
    if(current.getFname() == _tsVideo.at(_tsVideo.size() - 1).getFname())
    {
        SDL_CloseAudio();
        return false;
    }
    return true;
}