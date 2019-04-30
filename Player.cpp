#include "headers/Player.h"
#include "headers/Constants.h"

extern "C"
{
    #include "SDL2/SDL.h"
    #include "SDL2/SDL_ttf.h"
}

#include <iomanip>
#include <iostream>
#include <fstream>
#include <ios>
#include <thread>
#include <cassert>

void AudioReadFunc(void*, uint8_t*, int);

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
const uint32_t Player::SAMPLE_CORRECTION_PERCENT_MAX = 35;

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
    _pFrameYUV = av_frame_alloc();
    TTF_Init();
    std::thread(loadSegmentsThread, this).detach();
    av_log_set_level(AV_LOG_PANIC);
}

Player::~Player()
{
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
            toInsertVideo->appendData(segmentPayload + j * TS_BLOCK_SIZE, TS_BLOCK_SIZE, (i==0)?true:false, 
                (j == 0)?true:false, (*_playlist->getSegments())[i].getExtInf());
        }
        toInsertVideo->sizeAccumulate();
        if(i == 0)
        {
            toInsertVideo->prepareFile();
            toInsertVideo->prepareFormatContext(lastFormatContext);
            this->_tsVideo.push_back(toInsertVideo);
        }
    }
    toInsertVideo->finalizeLoading();
}

static uint8_t *audio_chunk; 
static uint32_t audio_len; 
static uint8_t *audio_pos;
static bool audio_clear = false;
bool playbackCompleted = false;

void  fill_audio(void* udata, uint8_t *stream, int len)
{
	SDL_memset(stream, 0, len);
	if(audio_len == 0)
		return; 

	len = ((uint32_t)len>audio_len ? audio_len : len);

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len; 
	audio_len -= len; 
}

uint32_t lastPoll = 0;

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
                if(lastPoll != 0)
                {
                    uint32_t currentPoll = SDL_GetTicks();
                    if(currentPoll - lastPoll < 500)
                    {
                        return true;
                    }
                    else
                    {
                        lastPoll = currentPoll;
                    }
                    
                }
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
                    actual->seek(-15, SEEK_CUR);
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                {
                    actual->seek(15, SEEK_CUR);
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_0 || event.key.keysym.scancode == SDL_SCANCODE_KP_0)
                    actual->seek(0, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_1 || event.key.keysym.scancode == SDL_SCANCODE_KP_1)
                    actual->seek(1, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_2 || event.key.keysym.scancode == SDL_SCANCODE_KP_2)
                    actual->seek(2, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_3 || event.key.keysym.scancode == SDL_SCANCODE_KP_3)
                    actual->seek(3, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_4 || event.key.keysym.scancode == SDL_SCANCODE_KP_4)
                    actual->seek(4, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_5 || event.key.keysym.scancode == SDL_SCANCODE_KP_5)
                    actual->seek(5, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_6 || event.key.keysym.scancode == SDL_SCANCODE_KP_6)
                    actual->seek(6, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_7 || event.key.keysym.scancode == SDL_SCANCODE_KP_7)
                    actual->seek(7, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_8 || event.key.keysym.scancode == SDL_SCANCODE_KP_8)
                    actual->seek(8, SEEK_SET);
                else if(event.key.keysym.scancode == SDL_SCANCODE_9 || event.key.keysym.scancode == SDL_SCANCODE_KP_9)
                    actual->seek(9, SEEK_SET);
            }
        default:  
            break;  
    }
    return true;
}

SDL_semaphore* audioSemaphore = SDL_CreateSemaphore(0);
volatile bool audioThreadRunning = true;


bool wasReset = false;

void audioThreadFunction(AVCodecContext* audioCodecContext, int* gotPicture, TSVideo* current, SwrContext* _swrCtx)
{
    SDL_PauseAudio(0);
    int32_t framesToSkip = 0;
    while(audioThreadRunning)
    {
        SDL_SemWait(audioSemaphore);
        AVFrame* _pFrame = av_frame_alloc();
        AVPacket* packet = current->dequeueAudio();
        if(packet == nullptr)
            continue;
        if(!audioThreadRunning)
        {
            std::cout << "audio ended early" << std::endl;
            return;
        }
        int ret = avcodec_decode_audio4(audioCodecContext, _pFrame, gotPicture, packet);
        if(ret < 0)
        {
            continue;
        }
        int outBufferSize = av_samples_get_buffer_size(NULL, audioCodecContext->channels, audioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);
        uint8_t* outBuffer = (uint8_t *)av_mallocz(outBufferSize * audioCodecContext->channels);
        if(gotPicture > 0)
        {
            swr_convert(_swrCtx, &outBuffer, outBufferSize, (const uint8_t **)_pFrame->data, _pFrame->nb_samples);
        }
            
        if(current->isResetAudio())
        {
            audio_clear = true;
            framesToSkip = current->getAudioQueueSize();
            uint8_t* tempBuffer = (uint8_t*)av_mallocz(outBufferSize);
            memcpy(tempBuffer, outBuffer, outBufferSize);
            av_free(audio_chunk);
            audio_chunk = (uint8_t*)tempBuffer;
            audio_len = outBufferSize;
            audio_pos = audio_chunk;
            current->clearResetAudio();
        }
        else
        {
            if(!(framesToSkip > 0))
            {
                uint8_t* tempBuffer;
                tempBuffer = (uint8_t*)av_mallocz(audio_len + outBufferSize);
                memcpy(tempBuffer, audio_pos, audio_len);
                memcpy(tempBuffer + audio_len, outBuffer, outBufferSize);
                av_free(audio_chunk);
                audio_chunk = (uint8_t*)tempBuffer;
		        audio_len = audio_len + outBufferSize;
                audio_pos = audio_chunk;
            }
            else
            {
                framesToSkip--;
            }
        }
        av_free(outBuffer);
        //av_free(packet);
    }
}

bool waitingForKeyFrame = false;
AVFrame* lastValidFrame = nullptr;

void videoThreadFunction(AVCodecContext* codecContext, SwsContext* _swsCtx, AVFrame* _pFrameYUV,
    SDL_Texture* _playerTexture, SDL_Rect* _sdlRect, SDL_Renderer* _playerRenderer, TSVideo* current,
    int32_t windowWidth, int32_t windowHeight, int32_t textWidth, int32_t textHeight)
{
    AVFrame* _pFrame = av_frame_alloc();
    AVPacket* packet = current->dequeueVideo();
    if(packet == nullptr)
        return;
    int frameFinished;
    int32_t startTicks = SDL_GetTicks();
    int decodeStatus = avcodec_decode_video2(codecContext, _pFrame, &frameFinished, packet);
    if(decodeStatus < 0)return;
    if(frameFinished)
    {
        if(!waitingForKeyFrame || _pFrame->key_frame == 1)
        {
            if(waitingForKeyFrame && _pFrame->key_frame == 1)
            {
                waitingForKeyFrame = false;
            }
            uint32_t seconds = _pFrame->pts / 100000;
            std::stringstream timestampString;
            timestampString << std::setfill('0') << std::setw(2) << seconds / 60 << ":" << std::setfill('0') << std::setw(2)  << seconds % 60
            << " / " << std::setfill('0') << std::setw(2) << current->getFullDuration() / 60 << ":" << std::setfill('0') << std::setw(2)
            << current->getFullDuration() % 60;

            current->setLastTimestamp(seconds);
            SDL_Surface *text;
            SDL_Color text_color = {255, 255, 255};
            text = TTF_RenderText_Solid(current->getFont(),
            timestampString.str().c_str(),
            text_color);

            if (text == NULL)
            {
                std::cerr << "TTF_RenderText_Solid() Failed: " << TTF_GetError() << std::endl;
                TTF_Quit();
                SDL_Quit();
                exit(1);
            }

            SDL_Texture* screenPrint = SDL_CreateTextureFromSurface(_playerRenderer, text);
            SDL_Rect messageRect;
            messageRect.x = windowWidth / 2 - textWidth / 2;
            messageRect.y = windowHeight - textHeight;
            messageRect.w = textWidth;
            messageRect.h = textHeight;


            sws_scale(  
            _swsCtx,  
            (uint8_t const * const *)_pFrame->data,  
            _pFrame->linesize,  
            0,  
            codecContext->height,
            _pFrameYUV->data,
            _pFrameYUV->linesize
            );
            if(_pFrame->key_frame == 1)lastValidFrame = _pFrameYUV;
            SDL_UpdateTexture(_playerTexture, _sdlRect, _pFrameYUV->data[0], _pFrameYUV->linesize[0]);
            SDL_RenderClear(_playerRenderer);
            SDL_RenderCopy(_playerRenderer, _playerTexture, _sdlRect, _sdlRect);
            
            SDL_RenderCopy(_playerRenderer, screenPrint, nullptr, &messageRect);
            SDL_RenderPresent(_playerRenderer);
            SDL_DestroyTexture(screenPrint);
            SDL_FreeSurface(text);
        }
        else
        {
            SDL_Surface *text;
            SDL_Color text_color = {255, 255, 255};
            text = TTF_RenderText_Solid(current->getFont(),
            "Seeking...",
            text_color);

            if (text == NULL)
            {
                std::cerr << "TTF_RenderText_Solid() Failed: " << TTF_GetError() << std::endl;
                TTF_Quit();
                SDL_Quit();
                exit(1);
            }

            SDL_Texture* screenPrint = SDL_CreateTextureFromSurface(_playerRenderer, text);
            SDL_Rect messageRect;
            messageRect.x = 0;
            messageRect.y = 0;
            messageRect.w = textWidth;
            messageRect.h = textHeight;


            sws_scale(  
            _swsCtx,  
            (uint8_t const * const *)_pFrame->data,  
            _pFrame->linesize,  
            0,  
            codecContext->height,
            _pFrameYUV->data,
            _pFrameYUV->linesize
            );
            SDL_RenderCopy(_playerRenderer, screenPrint, nullptr, &messageRect);
            SDL_RenderPresent(_playerRenderer);
            SDL_DestroyTexture(screenPrint);
            SDL_FreeSurface(text);

        }
        int32_t endTicks = SDL_GetTicks();
        int32_t delay = (1000.0 / av_q2d(codecContext->framerate)) - (endTicks - startTicks);
        delay = (delay < 0)?0:delay;
        if(delay == 0)delay = (_pFrame->pts - _pFrame->pkt_dts);
        delay = (delay < 0)?0:delay;
        SDL_Delay(delay);
    }
    //av_free(packet);
    av_free(_pFrame);
}

bool Player::playNext()
{
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
    if(!SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_RENDERER_ACCELERATED))
    {
        std::cout << "called init sdl" << std::endl;
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_RENDERER_ACCELERATED))
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
        wantedSpec.samples = 128;
        wantedSpec.callback = fill_audio;
        wantedSpec.userdata = current.getCustomIOContext();
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
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    current.assignVideoCodec(codecContext);
    current.assignAudioCodec(audioCodecContext);
    av_dump_format(_formatContext, 0, nullptr, 0);
    if(_swsCtx == nullptr)
    {
        _swsCtx = sws_getContext(
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            displayMode.w,
            displayMode.h,
            dst_fix_fmt,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);
  
        int numBytes = avpicture_get_size(  
            dst_fix_fmt,  
            displayMode.w,  
            displayMode.h);  
        uint8_t* buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));  
  
        avpicture_fill((AVPicture *)_pFrameYUV, buffer, dst_fix_fmt,  
            displayMode.w, displayMode.h);
        _sdlRect.x = 0;  
        _sdlRect.y = 0;  
        _sdlRect.w = displayMode.w;  
        _sdlRect.h = displayMode.h;
    }
    if(_playerWindow == nullptr)
    {
        SDL_GetCurrentDisplayMode(0, &displayMode);
        _playerWindow = SDL_CreateWindow("HLSPlayer",  
        SDL_WINDOWPOS_UNDEFINED,  
        SDL_WINDOWPOS_UNDEFINED,  
        displayMode.w,  displayMode.h,  
        0);
        if(!_playerWindow)
        {
            std::cerr << "SDL: could not set video mode" << std::endl;
            return false;
        }
        SDL_SetWindowFullscreen(_playerWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        _playerRenderer = SDL_CreateRenderer(_playerWindow, -1, SDL_RENDERER_TARGETTEXTURE);

        _playerTexture = SDL_CreateTexture(  
            _playerRenderer,
            SDL_PIXELFORMAT_BGR24,
            SDL_TEXTUREACCESS_STATIC,
            displayMode.w,  
            displayMode.h);  
	    if(!_playerTexture)
		    return false;
	    SDL_SetTextureBlendMode(_playerTexture,SDL_BLENDMODE_BLEND);
    }
    AVPacket packet;  
    SDL_Event event;
    int gotPicture = 0;
    std::thread audioThread = std::thread(audioThreadFunction, audioCodecContext, &gotPicture, &current, _swrCtx);
    audioThread.detach();
    bool enqueuedFirstAudio = false;
    while(av_read_frame(_formatContext, &packet) >= 0)
    {
        while(_paused)
        {
            if(!pollEvent(event, &current))
                return false;
        }
        if(current.isResetAudio())waitingForKeyFrame = true;
        SDL_LockMutex(current.getVideoPlayerMutex());
        if(packet.stream_index == _videoStream)
        {
            current.enqueueVideo(packet);
            if(enqueuedFirstAudio)videoThreadFunction(codecContext, _swsCtx, _pFrameYUV, _playerTexture, &_sdlRect, _playerRenderer, &current,
                displayMode.w, displayMode.h, displayMode.w / 10, displayMode.h / 10);
        }
        else if(packet.stream_index == _audioStream)
        {
            current.enqueueAudio(packet);
            SDL_SemPost(audioSemaphore);
            enqueuedFirstAudio = true;
        }
        SDL_UnlockMutex(current.getVideoPlayerMutex());
        if(!pollEvent(event, &current))
            return false;
    }
    audioThreadRunning = false;
    swr_free(&_swrCtx);   
    avcodec_close(codecContext);
    if(current.getFname() == _tsVideo.at(_tsVideo.size() - 1)->getFname())
    {
        SDL_CloseAudio();
        std::cout << "COMPLETED" << std::endl;
        return false;
    }
    return true;
}