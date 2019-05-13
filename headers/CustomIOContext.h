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

#include "PlaylistSegment.h"
#include "NetworkManager.h"

#ifndef _CUSTOM_IO_CONTEXT_H_
#define _CUSTOM_IO_CONTEXT_H_

class CustomIOContext {
private:
    SDL_mutex* _bufferMutex;

    NetworkManager* _networkManager;
    bool _resetAudio;
    AVIOContext* _ioCtx;
	uint8_t* _buffer;
	int _bufferSize;
    int32_t _block;
    int32_t _pos;
    int32_t _blockToSeek;
public:
    inline bool isResetAudio(){return _resetAudio;};
    inline void setResetAudio(){_resetAudio = true;};
    inline void clearResetAudio(){_resetAudio = false;};

    friend int IOReadFunc(void *data, uint8_t *buf, int buf_size);
    friend int64_t IOSeekFunc(void *data, int64_t offset, int whence);
    friend class TSVideo;
    friend class Player;
	CustomIOContext();
	~CustomIOContext();
	void initAVFormatContext(AVFormatContext *);
};


#endif