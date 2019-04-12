#include "headers/Player.h"
#include "headers/Constants.h"

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
    _paused = false;
    _formatContext = nullptr;
    _codec = nullptr;
    _audioCodec = nullptr;
    _swrCtx = nullptr;
    _swsCtx = nullptr;
    _videoStream = -1;
    _audioStream = -1;
    _pFrame = av_frame_alloc();  
    _pFrameYUV = av_frame_alloc();
    
    std::thread(loadSegmentsThread, this).detach();
    //av_log_set_level(AV_LOG_PANIC);
}

Player::~Player()
{
    av_free(_pFrame);  
    av_free(_pFrameYUV);
}

void Player::loadSegments()
{
    AVFormatContext* lastFormatContext = nullptr;
    TSVideo* toInsertVideo = new TSVideo((*_playlist->getSegments())[_playlist->getSegments()->size() - 1].getEndpoint());
    for(unsigned int i = 0; i < (*_playlist->getSegments()).size(); i++)
    {
        (*_playlist->getSegments())[i].loadSegment();
        uint8_t* segmentPayload = (*_playlist->getSegments())[i].getTsData();
        size_t payloadSize = (*_playlist->getSegments())[i].loadedSize();
        for(size_t j = 0; j < payloadSize / TS_BLOCK_SIZE; j++)
        {
            toInsertVideo->appendData(segmentPayload + j * TS_BLOCK_SIZE, TS_BLOCK_SIZE, (i==0)?true:false, (j == 0)?true:false);
        }
        if(i == 0)
        {
            toInsertVideo->prepareFile();
            toInsertVideo->prepareFormatContext(lastFormatContext);
            this->_tsVideo.push_back(toInsertVideo);
        }
    }
}

static uint8_t *audio_chunk; 
static uint32_t audio_len; 
static uint8_t *audio_pos;
static bool audio_clear = false;
bool gotFirstVideo = false;
bool gotFirstAudio = false;

void  fill_audio(void* udata, uint8_t *stream, int len)
{
    if(audio_clear)
    {
        SDL_ClearQueuedAudio(0);
        audio_clear = false;
    }
	SDL_memset(stream, 0, len);
	if(audio_len == 0)
		return; 

	len = ((uint32_t)len>audio_len ? audio_len : len);

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len; 
	audio_len -= len; 
}

bool Player::pollEvent(SDL_Event event, TSVideo* actual)
{
    SDL_PollEvent(&event);  
    switch(event.type)
    {  
        case SDL_QUIT:  
            SDL_Quit();  
            return false;
        case SDL_KEYDOWN:
            {
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_Quit();
                    return false;
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    _paused = !_paused;
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_LEFT)
                {
                    actual->seek(-2 * 1000000, SEEK_CUR);
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                {
                    actual->seek(2 * 1000000, SEEK_CUR);
                }
            }
        default:  
            break;  
    }
    return true;
}

bool Player::playNext()
{
    uint32_t oinputDelay = 0;
    uint32_t startTicks = SDL_GetTicks();
    bool firstFrame = true;
    while(!(*this->_playlist->getSegments())[_currentPosition].getIsLoaded());
    while(_currentPosition >= this->_tsVideo.size());
    TSVideo current = *this->_tsVideo[_currentPosition];
    _currentPosition++;
    _formatContext = current.getFormatContext();


    _formatContext->probesize = 32;
    _formatContext->max_analyze_duration = 32;

    if(_videoStream == -1 || _audioStream == -1) {      
        for(unsigned int i = 0; i < _formatContext->nb_streams; i++)
        {  
            if( _formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && _videoStream < 0)
            {  
                _videoStream = i;
            }
            if(_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && _audioStream < 0)
            {
                _audioStream = i;
            }
        }
    }  
    if(_videoStream == -1 || _audioStream == -1) {  
        std::cerr << "Failed finding stream" << std::endl;
        return false;
    }

    AVCodecContext* codecContext = _formatContext->streams[_videoStream]->codec;
    AVCodecContext* audioCodecContextOrig = _formatContext->streams[_audioStream]->codec;
    AVCodecContext* audioCodecContext;

    if(_codec == nullptr || _audioCodec == nullptr) {
        std::cout << "Called find decoder" << std::endl;
        _codec = avcodec_find_decoder(codecContext->codec_id);
        _audioCodec = avcodec_find_decoder(audioCodecContextOrig->codec_id);
    }
    if(_codec == nullptr || _audioCodec == nullptr) {  
        std::cerr << "Unsupported _codec" << std::endl;  
        return false;
    }
    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec audioSpec;
    if(!SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        std::cout << "called init sdl" << std::endl;
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
        {
            std::cerr << "Could not initialize SDL - " << SDL_GetError() << std::endl;
            return false;
        }
        audioCodecContext = avcodec_alloc_context3(_audioCodec);

        if(avcodec_copy_context(audioCodecContext, audioCodecContextOrig) != 0)
        {
            std::cerr << "Couldn't copy audio _codec context" << std::endl;
            return false;
        }
        wantedSpec.freq = audioCodecContext->sample_rate;
        wantedSpec.format = AUDIO_S16;
        wantedSpec.channels = audioCodecContext->channels;
        wantedSpec.silence = 0;
        wantedSpec.samples = 16;
        wantedSpec.callback = fill_audio;
        wantedSpec.userdata = audioCodecContext;
        if(SDL_OpenAudio(&wantedSpec, &audioSpec) < 0)
        {
            std::cerr << "SDL_OpenAudio " << SDL_GetError() << std::endl;
            return false;
        }
    }

    if(_swrCtx == nullptr)
    {
        _inChLayout = av_get_default_channel_layout(audioCodecContext->channels);
        _swrCtx = swr_alloc();
        _swrCtx = swr_alloc_set_opts(_swrCtx, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, audioCodecContext->sample_rate, _inChLayout, audioCodecContext->sample_fmt,
            audioCodecContext->sample_rate, 0, nullptr);
        swr_init(_swrCtx);

        SDL_PauseAudio(0);
    }
    AVDictionary* optionsDictionary = nullptr;
    if(avcodec_open2(codecContext, _codec, &optionsDictionary) < 0 || avcodec_open2(audioCodecContext, _audioCodec, nullptr) < 0)
    {  
        std::cerr << "Could not open _codec" << std::endl;
        return false; 
    }
	AVPixelFormat dst_fix_fmt = AV_PIX_FMT_BGR24;
    if(_pFrameYUV == nullptr) {
        std::cerr << "Bad frame format" << std::endl;
        return false;  
    }

    if(_swsCtx == nullptr)
    {
        _swsCtx = sws_getContext(
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
  
        avpicture_fill((AVPicture *)_pFrameYUV, buffer, dst_fix_fmt,  
            codecContext->width, codecContext->height);
        _sdlRect.x = 0;  
        _sdlRect.y = 0;  
        _sdlRect.w = codecContext->width;  
        _sdlRect.h = codecContext->height;
    }
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
        //SDL_SetWindowFullscreen(_playerWindow, SDL_WINDOW_FULLSCREEN);
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

    uint32_t endTicks = SDL_GetTicks();
    oinputDelay = endTicks - startTicks;
    int gotPicture;
    uint32_t videoDelay = 0;
    uint32_t audioDelay = 0;
    while(av_read_frame(_formatContext, &packet) >= 0)
    {
        while(_paused)
        {
            if(!pollEvent(event, &current))
                return false;
        }
        uint32_t startTicks = SDL_GetTicks();
        if(packet.stream_index == _videoStream)
        {
            int frameFinished;
            avcodec_decode_video2(codecContext, _pFrame, &frameFinished, &packet);
            if(frameFinished)
            {
                gotFirstVideo = true;
                if(gotFirstVideo && gotFirstAudio)
                {
                    sws_scale(  
                    _swsCtx,  
                    (uint8_t const * const *)_pFrame->data,  
                    _pFrame->linesize,  
                    0,  
                    codecContext->height,  
                    _pFrameYUV->data,
                    _pFrameYUV->linesize  
                    );  
                  
                    SDL_UpdateTexture(_playerTexture, &_sdlRect, _pFrameYUV->data[0], _pFrameYUV->linesize[0]);  
                    SDL_RenderClear(_playerRenderer);  
                    SDL_RenderCopy(_playerRenderer, _playerTexture, &_sdlRect, &_sdlRect);  
                    SDL_RenderPresent(_playerRenderer);
                }
            }
            
            if(frameFinished)
            {
                int framerate = av_q2d(codecContext->framerate);
                uint32_t endTicks = SDL_GetTicks();
                videoDelay = (uint32_t)(1000/(double)framerate) - (endTicks - startTicks);
                if(firstFrame)
                {
                    videoDelay -= (videoDelay > oinputDelay)?oinputDelay : videoDelay;
                    if(gotFirstAudio && gotFirstVideo)firstFrame = false;
                }
                videoDelay -= audioDelay;
                audioDelay = 0;
                SDL_Delay(videoDelay);
            }
        }
        else if(packet.stream_index == _audioStream && gotFirstVideo)
        {
            uint32_t audioStartTicks = SDL_GetTicks();
            int ret = avcodec_decode_audio4(audioCodecContext, _pFrame, &gotPicture, &packet);
            if(ret < 0)
            {
                std::cerr << "Error decoding audio" << std::endl;
                continue;
            }
            int outBufferSize = av_samples_get_buffer_size(NULL,1 , audioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);
            uint8_t* outBuffer = (uint8_t *)av_malloc(192000 * 2);
            if(gotPicture > 0)
            {
                swr_convert(_swrCtx, &outBuffer, 192000, (const uint8_t **)_pFrame->data, _pFrame->nb_samples);
            }
            if(current.isResetAudio())
            {
                audio_clear = true;
                std::cout << "Resetting audio" << std::endl;
                uint8_t* tempBuffer = (uint8_t*)av_malloc(outBufferSize);
                memcpy(tempBuffer, outBuffer, outBufferSize);
                av_free(audio_chunk);
                audio_chunk = (uint8_t*)tempBuffer;
                audio_len = outBufferSize;
                audio_pos = audio_chunk;
                current.clearResetAudio();
            }
            else
            {
                uint8_t* tempBuffer = (uint8_t*)av_malloc(audio_len + outBufferSize);
                memcpy(tempBuffer, audio_pos, audio_len);
                memcpy(tempBuffer + audio_len, outBuffer, outBufferSize);
                av_free(audio_chunk);
                audio_chunk = (uint8_t*)tempBuffer;
			    audio_len = audio_len + outBufferSize;
                audio_pos = audio_chunk;
            }
            av_free(outBuffer);
            gotFirstAudio = true;
            uint32_t audioEndTicks = SDL_GetTicks();
            audioDelay = audioEndTicks - audioStartTicks;
            SDL_Delay(audioDelay);
        }
        av_free_packet(&packet);
        if(!pollEvent(event, &current))
            return false;
    }
    swr_free(&_swrCtx);   
    avcodec_close(codecContext);  
    //avformat_close_input(&_formatContext);
    gotFirstVideo = false;
    gotFirstAudio = false;
    if(current.getFname() == _tsVideo.at(_tsVideo.size() - 1)->getFname())
    {
        SDL_CloseAudio();
        return false;
    }
    return true;
}