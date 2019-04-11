#include <cstdio>
#include <string>

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
    AVIOContext* _ioCtx;
	uint8_t* _buffer;
	int _bufferSize;
    int32_t _pos;

public:
	CustomIOContext(uint8_t* buffer, size_t size);
	~CustomIOContext();
	void initAVFormatContext(AVFormatContext *);
};


#endif