#include <cstdio>
#include <string>
#include <vector>

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

#ifndef _CUSTOM_IO_CONTEXT_H_
#define _CUSTOM_IO_CONTEXT_H_

class CustomIOContext {
public:
    bool _resetAudio;
    AVIOContext* _ioCtx;
	uint8_t* _buffer;
	int _bufferSize;
    uint8_t* _videoBuffer;
    int _videoBufferSize;
    int32_t _pos;
	CustomIOContext();
	~CustomIOContext();
	void initAVFormatContext(AVFormatContext *);
};


#endif