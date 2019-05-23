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
#include <map>

void AudioReadFunc(void*, uint8_t*, int);

const uint32_t Player::VIDEO_PID_VAL = 0x100;
const uint32_t Player::AUDIO_PID_VAL = 0x101;
const uint32_t Player::PID_MASK = 0x1fff00;
const uint32_t Player::ADAPTATION_FIELD_MASK = 0x30;
const uint32_t Player::ADAPTATION_NO_PAYLOAD = 0b10;
const uint32_t Player::ADAPTATION_ONLY_PAYLOAD = 0b01;
const uint32_t Player::ADAPTATION_BOTH = 0b11;
const uint32_t Player::SAMPLE_CORRECTION_PERCENT_MAX = 35;

Player::Player(Playlist* playlist, int32_t width, int32_t height, int32_t maxMemory, bool fullScreen)
{
    this->_playlist = playlist;
    this->_currentPosition = 0;
    this->_playerWindow = nullptr;
    this->_playerRenderer = nullptr;
    this->_desiredWidth = width;
    this->_desiredHeight = height;
    this->_desiredMaxMemory = maxMemory;
    this->_desiredFullScreen = fullScreen;
    this->_statusMessage = " ";
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
    _lastPoll = 0;
    _pFrameYUV = av_frame_alloc();
    TTF_Init();
    av_log_set_level(AV_LOG_PANIC);
}

Player::Player(std::vector<Playlist*>* playlists, std::vector<int32_t>* bitrates, int32_t width, int32_t height, int32_t maxMemory, bool fullScreen)
{
    this->_playlists = playlists;
    this->_bitrates = bitrates;
    this->_currentPosition = 0;
    this->_playerWindow = nullptr;
    this->_playerRenderer = nullptr;
    this->_desiredWidth = width;
    this->_desiredHeight = height;
    this->_desiredMaxMemory = maxMemory;
    this->_desiredFullScreen = fullScreen;
    this->_statusMessage = " ";
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
    av_log_set_level(AV_LOG_PANIC);
}

Player::~Player()
{
    av_free(_pFrameYUV);
}

void Player::loadSegments()
{
    this->_tsVideo = new TSVideo();
	this->_tsVideo->_ioCtx->_networkManager = new NetworkManager(_playlists, _bitrates, _desiredMaxMemory, 10, 1);
    this->_tsVideo->prepareFormatContext();
	this->_tsVideo->_ioCtx->_networkManager->start();
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
                uint32_t currentTicks = SDL_GetTicks();
                if(!this->_tsVideo->_acceptsInterrupts || (currentTicks - _tsVideo->_lastPoll < 1000 && _tsVideo->_lastPoll != 0))
                {
                    return true;
                }
                else
                {
                    this->_lastPoll = SDL_GetTicks();
                }
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    SDL_Quit();
                    return false;
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    _paused = !_paused;
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_LEFT)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(-15, SEEK_CUR, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(15, SEEK_CUR, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_0 || event.key.keysym.scancode == SDL_SCANCODE_KP_0)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(0, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_1 || event.key.keysym.scancode == SDL_SCANCODE_KP_1)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(1, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_2 || event.key.keysym.scancode == SDL_SCANCODE_KP_2)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(2, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_3 || event.key.keysym.scancode == SDL_SCANCODE_KP_3)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(3, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_4 || event.key.keysym.scancode == SDL_SCANCODE_KP_4)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(4, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_5 || event.key.keysym.scancode == SDL_SCANCODE_KP_5)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(5, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_6 || event.key.keysym.scancode == SDL_SCANCODE_KP_6)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(6, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_7 || event.key.keysym.scancode == SDL_SCANCODE_KP_7)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(7, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_8 || event.key.keysym.scancode == SDL_SCANCODE_KP_8)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(8, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_9 || event.key.keysym.scancode == SDL_SCANCODE_KP_9)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Seek        ";
                    actual->seek(9, SEEK_SET, actual->getLastTimestamp());
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_KP_PLUS)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_tsVideo->setVolumeRate((this->_tsVideo->getVolumeRate() < 10)?this->_tsVideo->getVolumeRate() + 1 :
                    this->_tsVideo->getVolumeRate());
                    this->_statusMessage = "Volume:  " + std::to_string(this->_tsVideo->getVolumeRate() * 10) + "%";
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_KP_MINUS)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_tsVideo->setVolumeRate((this->_tsVideo->getVolumeRate() > 0)?this->_tsVideo->getVolumeRate() - 1 : 
                    this->_tsVideo->getVolumeRate());
                    this->_statusMessage = "Volume:  " + std::to_string(this->_tsVideo->getVolumeRate() * 10) + "%";
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_UP)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Bitrate: " + std::to_string(this->_tsVideo->setManualBitrate(1) / 1000) + "Kbps";
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_DOWN)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Bitrate: " + std::to_string(this->_tsVideo->setManualBitrate(-1) / 1000) + "Kbps";
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_A)
                {
                    this->_messageSetTimestamp = SDL_GetTicks();
                    this->_statusMessage = "Bitrate: Automatic";
                    this->_tsVideo->setAutomaticBitrate();
                }
            }
        default:  
            break;  
    }
    SDL_FlushEvent(SDL_KEYDOWN);
    SDL_FlushEvent(SDL_KEYUP);
    return true;
}

void audioThreadFunction(AVCodecContext* audioCodecContext, int* gotPicture, TSVideo* current, SwrContext* _swrCtx)
{
    SDL_PauseAudio(0);
    while(current->getAudioThreadRunning())
    {
        //SDL_SemWait(current->getAudioSemaphore());
        AVFrame* _pFrame = av_frame_alloc();
        AVPacket packet = current->dequeueAudio();
        if(packet.data == nullptr || packet.data == NULL)
        {
            av_frame_free(&_pFrame);
            continue;
        }
        if(!current->getAudioThreadRunning())
        {
            av_frame_free(&_pFrame);
            return;
        }
        if(packet.side_data_elems != 0)std::cout << "SIDE_DATA_ELEMS:" << packet.side_data_elems << std::endl;
        int ret = avcodec_decode_audio4(audioCodecContext, _pFrame, gotPicture, &packet);
        if(ret < 0)
        {
            av_frame_free(&_pFrame);
            continue;
        }
        int outBufferSize = av_samples_get_buffer_size(NULL, audioCodecContext->channels, audioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);
        uint8_t* outBuffer = (uint8_t *)av_mallocz(outBufferSize * audioCodecContext->channels * 3 / 2);
        if(gotPicture > 0)
        {
            swr_convert(_swrCtx, &outBuffer, outBufferSize, (const uint8_t **)_pFrame->data, _pFrame->nb_samples);
        }
            
        SDL_LockMutex(current->getAudioMutex());
        if(current->isResetAudio())
        {
            current->setEnqueuedFirstAudio(false);
            current->setFramesToSkip(current->getFramesToSkip() + current->getAudioQueueSize());
            current->setAudioPos(current->getAudioPos() + current->getAudioLen());
            current->setAudioLen(0);
            current->clearResetAudio();
        }
        else
        {
            current->setReferencePts(_pFrame->pts);
            uint8_t* tempBuffer;
            tempBuffer = (uint8_t*)av_mallocz(current->getAudioLen() + outBufferSize);
            memcpy(tempBuffer, current->getAudioPos(), current->getAudioLen());
            memcpy(tempBuffer + current->getAudioLen(), outBuffer, outBufferSize);
            av_free(current->getAudioChunk());
            current->setAudioChunk((uint8_t*)tempBuffer);
            current->setAudioLen(current->getAudioLen() + outBufferSize);
            current->setAudioPos(current->getAudioChunk());
        }
        SDL_UnlockMutex(current->getAudioMutex());
        av_free(outBuffer);
        av_frame_free(&_pFrame);
    }
}

void videoFunction(AVCodecContext* codecContext, SwsContext* _swsCtx, AVFrame* _pFrameYUV,
    SDL_Texture* _playerTexture, SDL_Rect* _sdlRect, SDL_Renderer* _playerRenderer, TSVideo* current,
    int32_t windowWidth, int32_t windowHeight, int32_t textWidth, int32_t textHeight, Player* player)
{
    AVFrame* _pFrame = av_frame_alloc();
    AVPacket packet = current->dequeueVideo();
    if(packet.data == nullptr || packet.data == NULL)
    {
        av_frame_free(&_pFrame);
        return;
    }
    int frameFinished;
    int32_t startTicks = SDL_GetTicks();
    int decodeStatus = avcodec_decode_video2(codecContext, _pFrame, &frameFinished, &packet);
    if(decodeStatus < 0)
    {
        av_frame_free(&_pFrame);
        return;
    }
    if(frameFinished)
    {
        double windowAspectRatio = (double)windowWidth / (double)windowHeight;
        double videoAspectRatio = (double)_pFrame->width / (double)_pFrame->height;
        bool windowWider = windowAspectRatio > videoAspectRatio;
        int32_t desiredVideoWidth = 0;
        int32_t desiredVideoHeight = 0;
        if(windowWider)
        {
            double heightWindowToVideoRatio = (double)windowHeight / (double)_pFrame->height;
            desiredVideoHeight = windowHeight;
            desiredVideoWidth = _pFrame->width * heightWindowToVideoRatio;
        }
        else
        {
            double widthWindowToVideoRatio = (double)windowWidth / (double)_pFrame->width;
            desiredVideoWidth = windowWidth;
            desiredVideoHeight = _pFrame->height * widthWindowToVideoRatio;
        }
        SDL_Surface *text;
        SDL_Color text_color = {255, 255, 255};
        if(SDL_GetTicks() - player->_messageSetTimestamp > 1500)player->_statusMessage = " ";
        text = TTF_RenderText_Solid(current->getFont(),
        player->_statusMessage.c_str(),
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
        _swsCtx = sws_getContext(
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            desiredVideoWidth,
            desiredVideoHeight,
            AV_PIX_FMT_BGR24,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);

        sws_scale(  
            _swsCtx,  
            (uint8_t const * const *)_pFrame->data,  
            _pFrame->linesize,  
            0,  
            codecContext->height,
            _pFrameYUV->data,
            _pFrameYUV->linesize
        );
        if(windowWider)
        {
            _sdlRect->x = (windowWidth - desiredVideoWidth) / 2;
            _sdlRect->y = 0;
            _sdlRect->w = desiredVideoWidth;
            _sdlRect->h = desiredVideoHeight;
        }
        else
        {
            _sdlRect->x = 0;
            _sdlRect->y = (windowHeight - desiredVideoHeight) / 2;
            _sdlRect->w = desiredVideoWidth;
            _sdlRect->h = desiredVideoHeight;
        }
        SDL_UpdateTexture(_playerTexture, _sdlRect, _pFrameYUV->data[0], _pFrameYUV->linesize[0]);
        SDL_RenderClear(_playerRenderer);
        SDL_RenderCopy(_playerRenderer, _playerTexture, _sdlRect, _sdlRect);
        
        SDL_RenderCopy(_playerRenderer, screenPrint, NULL, &messageRect);
        SDL_RenderPresent(_playerRenderer);
        SDL_DestroyTexture(screenPrint);
        SDL_FreeSurface(text);
        sws_freeContext(_swsCtx);
        
        int32_t endTicks = SDL_GetTicks();
        int32_t maxDelay = (1000.0 / av_q2d(codecContext->framerate)) * 5;
        int32_t delay = (1000.0 / av_q2d(codecContext->framerate)) - (endTicks - startTicks);
        delay = (delay < 0)?0:delay;
        int32_t avDesync = (current->getReferencePts() - _pFrame->pts) / 10000;
        current->setUnderMsDelay(current->getUnderMsDelay() + (current->getReferencePts() - _pFrame->pts) % 10000);
        if(abs(current->getUnderMsDelay()) > 10000)
        {
            avDesync += current->getUnderMsDelay() / 10000;
            current->setUnderMsDelay(current->getUnderMsDelay() % 10000);
        }
        delay += (avDesync);
        if(delay < 0)
        {
            current->setFramesToSkip(current->getFramesToSkip() - 1);
            delay = 0;
        }
        if(delay > maxDelay)
        {
            current->setFramesToSkip(current->getFramesToSkip() + 1);
            delay = maxDelay;
        }
        SDL_Delay(delay);
    }
    av_frame_free(&_pFrame);
}

bool Player::play()
{
    TSVideo* current = this->_tsVideo;
    _currentPosition++;
    _formatContext = current->getFormatContext();


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
        wantedSpec.silence = 1;
        wantedSpec.samples = 128;
        wantedSpec.callback = AudioReadFunc;
        wantedSpec.userdata = current->getCustomIOContext();
        if(SDL_OpenAudio(&wantedSpec, &audioSpec) < 0)
        {
            std::cerr << "SDL_OpenAudio " << SDL_GetError() << std::endl;
            return false;
        }
    }

    if(avcodec_open2(codecContext, _codec, NULL) < 0 || avcodec_open2(audioCodecContext, _audioCodec, NULL) < 0)
    {  
        std::cerr << "Could not open codecs" << std::endl;
        return false; 
    }
    if(_swrCtx == nullptr)
    {
        _inChLayout = av_get_default_channel_layout(audioCodecContext->channels);
        _swrCtx = swr_alloc();
        _swrCtx = swr_alloc_set_opts(NULL, audioCodecContext->channel_layout, AV_SAMPLE_FMT_S16, audioCodecContext->sample_rate, _inChLayout, audioCodecContext->sample_fmt,
            audioCodecContext->sample_rate, 0, NULL);
        swr_init(_swrCtx);
    }
	AVPixelFormat dst_fix_fmt = AV_PIX_FMT_BGR24;
    if(_pFrameYUV == nullptr)
    {
        std::cerr << "Bad frame format" << std::endl;
        return false;  
    }
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    if(!this->_desiredFullScreen && this->_desiredWidth > 0)displayMode.w = this->_desiredWidth;
    if(!this->_desiredFullScreen && this->_desiredHeight > 0)displayMode.h = this->_desiredHeight;
    current->assignVideoCodec(codecContext);
    current->assignAudioCodec(audioCodecContext);
    av_dump_format(_formatContext, 0, NULL, 0);
    if(_swsCtx == nullptr)
    {
        _swsCtx = sws_getContext(
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            displayMode.w,
            displayMode.h,
            dst_fix_fmt,
            SWS_BICUBIC,
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
        if(!this->_desiredFullScreen && this->_desiredWidth > 0)displayMode.w = this->_desiredWidth;
        if(!this->_desiredFullScreen && this->_desiredHeight > 0)displayMode.h = this->_desiredHeight;
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
        if(this->_desiredFullScreen)SDL_SetWindowFullscreen(_playerWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        _playerRenderer = SDL_CreateRenderer(_playerWindow, -1, SDL_RENDERER_TARGETTEXTURE || SDL_HINT_RENDER_VSYNC);

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
    std::thread audioThread = std::thread(audioThreadFunction, audioCodecContext, &gotPicture, current, _swrCtx);
    audioThread.detach();
    if(this->_desiredFullScreen)SDL_ShowCursor(SDL_DISABLE);
    while(av_read_frame(_formatContext, &packet) >= 0)
    {
        while(_paused)
        {
            if(!pollEvent(event, current))
                return false;
        }
        SDL_LockMutex(current->getVideoPlayerMutex());
        if(packet.stream_index == _videoStream)
        {
            current->enqueueVideo(packet);
            if(current->getEnqueuedFirstAudio())
            {
                if(this->_desiredFullScreen)
                {
                    videoFunction(codecContext, _swsCtx, _pFrameYUV, _playerTexture, &_sdlRect, _playerRenderer, current,
                        displayMode.w, displayMode.h, displayMode.w / 10, displayMode.h / 10, this);
                }
                else
                {
                    videoFunction(codecContext, _swsCtx, _pFrameYUV, _playerTexture, &_sdlRect, _playerRenderer, current,
                        this->_desiredWidth, this->_desiredHeight, this->_desiredWidth / 6, this->_desiredHeight / 10, this);
                }
                
            }
        }
        else if(packet.stream_index == _audioStream)
        {
            current->enqueueAudio(packet);
            SDL_SemPost(current->getAudioSemaphore());
            current->setEnqueuedFirstAudio(true);
        }
        SDL_UnlockMutex(current->getVideoPlayerMutex());
        if(!pollEvent(event, current))
            return false;
    }
    current->setAudioThreadRunning(false);
    SDL_ShowCursor(SDL_ENABLE);
    swr_free(&_swrCtx);   
    avcodec_close(codecContext);
    SDL_CloseAudio();
    std::cout << "COMPLETED" << std::endl;
    return false;
}

int32_t Player::prepare()
{
    std::map<int32_t, int32_t> occurences;
    for(int32_t i = 0; i < (int32_t)_playlists->size(); i++)
    {
        occurences[(int32_t)_playlists->at(i)->getSegments()->size()]++;
    }
    int32_t max = 0;
    int32_t toReturn = 0;
    for(std::map<int32_t, int32_t>::iterator iter = occurences.begin(); iter != occurences.end(); ++iter)
    {
       if(iter->second > max)toReturn = iter->first;
    }
    _playlists->clear();
    _bitrates->clear();
    return toReturn;
}