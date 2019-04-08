#include "headers/Player.h"
#include "headers/Constants.h"

extern "C"  
{ 
    #include "libavcodec/avcodec.h"  
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"  
    
    #include "SDL2/SDL.h"  
    #include "SDL2/SDL_thread.h"
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

typedef struct PacketQueue
{
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
}PacketQueue;

int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;
    if(av_dup_packet(pkt) < 0)
    {
        return -1;
    }
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
    {
        return -1;
    }
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    SDL_LockMutex(q->mutex);
    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);
    return 0;
}

int quit = 0;

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int ret;
    SDL_LockMutex(q->mutex);
    for(;;)
    {
        if(quit)
        {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1)
        {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
	        q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        }
        else if(!block)
        {
            ret = 0;
            break;
        }
        else
        {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}


PacketQueue* audioq;

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size)
{
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;
    int len1, data_size = 0;
    for(;;) 
    {
        while(audio_pkt_size > 0)
        {
            int got_frame = 0;

            len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
            if(len1 < 0)
            {
	            audio_pkt_size = 0;
	            break;
            }
        audio_pkt_data += len1;
        audio_pkt_size -= len1;
        data_size = 0;
        if(got_frame)
        {
	        data_size = av_samples_get_buffer_size(NULL, aCodecCtx->channels, frame.nb_samples, aCodecCtx->sample_fmt, 1);
	        assert(data_size <= buf_size);
	        memcpy(audio_buf, frame.data[0], data_size);
        }
        if(data_size <= 0)
        {
            continue;
        }
        return data_size;
        }
        if(pkt.data)
            av_free_packet(&pkt);
        if(quit)
        {
            return -1;
        }
        if(packet_queue_get(audioq, &pkt, 1) < 0)
        {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

void audioCallback(void *userdata, Uint8 *stream, int len)
{
    AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(192000 * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;
    while(len > 0)
    {
        if(audio_buf_index >= audio_buf_size) 
        {
            audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
            if(audio_size < 0)
            {
	            audio_buf_size = 1024;
	            memset(audio_buf, 0, audio_buf_size);
            }
            else
            {
	            audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
  }
}

void packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
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
    audioq = new PacketQueue();
    std::thread(loadSegmentsThread, this).detach();
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
                toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
                
            }
            else if (pid == AUDIO_PID_VAL)
            {
                toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
            }
            else
            {
                //std::cout << "pid:" << pid << std::endl;
            }
        }
        this->_tsVideo.push_back(toInsertVideo);
        this->_tsAudio.push_back(toInsertAudio);
    }
}

void Player::loadSegment(uint32_t pos)
{
    (*_playlist->getSegments())[pos].loadSegment();
    uint8_t* segmentPayload = (*_playlist->getSegments())[pos].getTsData();
    size_t payloadSize = (*_playlist->getSegments())[pos].loadedSize();
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
            toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
        }
        else if (pid == AUDIO_PID_VAL)
        {
            toInsertVideo.appendData(segmentPayload + i * TS_BLOCK_SIZE, TS_BLOCK_SIZE);
        }
        else
        {
            //std::cout << "pid:" << pid << std::endl;
        }
    }
    this->_tsVideo.push_back(toInsertVideo);
    this->_tsAudio.push_back(toInsertAudio);
}

bool Player::playNext()
{
    packet_queue_init(audioq);
    //SDL_PauseAudio(0);
    //av_log_set_level(AV_LOG_PANIC);
    while(!(*this->_playlist->getSegments())[_currentPosition].getIsLoaded());
    while(_currentPosition >= this->_tsVideo.size());
    TSVideo current = this->_tsVideo[_currentPosition];
    TSAudio currentAudio = this->_tsAudio[_currentPosition];
    _currentPosition++;
    uint8_t* payload = current.getPayload();
    //uint8_t* audioPayload = currentAudio.getPayload();
    std::ofstream tmpfile;
    //std::ofstream tmpAudioFile;
    tmpfile.open("tmp", std::ios::out | std::ios::binary);
    //tmpAudioFile.open("tmpAudio", std::ios::out | std::ios::binary);
    tmpfile.write((char*)payload, current.getSize());
    //tmpfile.write((char*)audioPayload, currentAudio.getSize());

    av_register_all();
    AVFormatContext *formatContext = nullptr;
    if(avformat_open_input(&formatContext, "tmp", nullptr, nullptr) != 0)
    {
        std::cerr << "Format context failed" << std::endl;
        return false;
    }

    if(avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        std::cerr << "Stream info failed" << std::endl;
        return false;
    }

    av_dump_format(formatContext, 0, "tmp", 0);

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

    audioCodecContext = avcodec_alloc_context3(audioCodec);

    if(avcodec_copy_context(audioCodecContext, audioCodecContextOrig) != 0)
    {
        std::cerr << "Couldn't copy audio codec context" << std::endl;
        return false;
    }

    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec audioSpec;
    wantedSpec.freq = audioCodecContext->sample_rate;
    wantedSpec.format = AUDIO_S16SYS;
    wantedSpec.channels = audioCodecContext->channels;
    wantedSpec.silence = 1;
    wantedSpec.samples = (uint16_t)882000;
    wantedSpec.callback = audioCallback;
    wantedSpec.userdata = audioCodecContext;
    if(SDL_OpenAudio(&wantedSpec, &audioSpec) < 0)
    {
        std::cerr << "SDL_OpenAudio " << SDL_GetError() << std::endl;
        return false;
    }
    AVDictionary* optionsDictionary = nullptr;
    std::cout << "Samples:" << audioSpec.samples << std::endl;

    if(avcodec_open2(codecContext, codec, &optionsDictionary) < 0 || avcodec_open2(audioCodecContextOrig, audioCodec, nullptr) < 0)
    {  
        std::cerr << "Could not open codec" << std::endl;
        return false; 
    }

    //AVPixelFormat src_fix_fmt = codecContext->pix_fmt;
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
    //std::cout << "W:" << codecContext->width << " H:" << codecContext->height << std::endl;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        std::cerr << "Could not initialize SDL - " << SDL_GetError() << std::endl;
        return false;
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
        _playerRenderer = SDL_CreateRenderer(_playerWindow, -1, SDL_RENDERER_TARGETTEXTURE);
    }

    SDL_Texture* sdlTexture = SDL_CreateTexture(  
        _playerRenderer,  
        SDL_PIXELFORMAT_BGR24,  
        SDL_TEXTUREACCESS_STATIC, 
        codecContext->width,  
        codecContext->height);  
	if(!sdlTexture)
		return -1;
	SDL_SetTextureBlendMode(sdlTexture,SDL_BLENDMODE_BLEND );
    AVPacket packet;  
    SDL_Event event;

    while(av_read_frame(formatContext, &packet) >= 0)
    { 
        if(packet.stream_index == videoStream)
        {
            uint32_t startTicks = SDL_GetTicks();
            int frameFinished;  
            avcodec_decode_video2(codecContext, pFrame, &frameFinished, &packet);
            if(frameFinished)
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
                  
                SDL_UpdateTexture(sdlTexture, &sdlRect, pFrameYUV->data[0], pFrameYUV->linesize[0]);  
                SDL_RenderClear(_playerRenderer);  
                SDL_RenderCopy(_playerRenderer, sdlTexture, &sdlRect, &sdlRect);  
                SDL_RenderPresent(_playerRenderer);
            }
            int framerate = av_q2d(codecContext->framerate);
            uint32_t endTicks = SDL_GetTicks();
            SDL_Delay((int)(1000/(double)framerate) - (endTicks - startTicks));
        }
        else if(packet.stream_index == audioStream)
        {
            packet_queue_put(audioq, &packet);
            SDL_PollEvent(&event);
            switch(event.type)
            {
                case SDL_QUIT:
                    quit = 1;
            }
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
  
    
    av_free(pFrame);  
    av_free(pFrameYUV);   
    avcodec_close(codecContext);  
    avformat_close_input(&formatContext);
    SDL_CloseAudio();
    return true;
}