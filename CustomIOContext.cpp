#include "headers/CustomIOContext.h"
extern "C"
{
	#include "libavutil/error.h"
}
#include <iostream>

int IOReadFunc(void *data, uint8_t *buf, int buf_size)
{
	CustomIOContext *hctx = (CustomIOContext*)data;
	if (hctx->_pos >= hctx->_videoBufferSize)
    {
		return AVERROR_EOF;
	}
	SDL_UnlockMutex(hctx->_bufferMutex);
	int32_t toRead = (buf_size < 16384)?buf_size:16384;
	toRead = (toRead < (hctx->_videoBufferSize - hctx->_pos))?toRead:(hctx->_videoBufferSize - hctx->_pos);
	if(toRead <= 0)
	{
		std::cout << "toRead <= 0" << std::endl;
		SDL_UnlockMutex(hctx->_bufferMutex);
		return AVERROR_EOF;
	}
	memcpy(buf, hctx->_videoBuffer + hctx->_pos, toRead);
	hctx->_pos += toRead;
	SDL_UnlockMutex(hctx->_bufferMutex);
	return toRead;
}

int64_t IOSeekFunc(void *data, int64_t offset, int whence)
{
	CustomIOContext* hctx = (CustomIOContext*)data;
	if(hctx->_ioCtx == nullptr)
	{
		std::cerr << "IO Context Missing" << std::endl;
		return -1;
	}
	SDL_LockMutex(hctx->_bufferMutex);
	if(whence == SEEK_SET)
	{
		hctx->_resetAudio = true;
		avio_flush(hctx->_ioCtx);
		if(offset < 0)
		{
			hctx->_pos = 0;
			SDL_UnlockMutex(hctx->_bufferMutex);
			return 0;
		}
		else if(offset > hctx->_videoBufferSize)
		{
			hctx->_pos = hctx->_videoBufferSize;
			SDL_UnlockMutex(hctx->_bufferMutex);
			return hctx->_pos;
		}
		else
		{
			hctx->_pos = offset;
			SDL_UnlockMutex(hctx->_bufferMutex);
			return offset;
		}
	}
	SDL_UnlockMutex(hctx->_bufferMutex);
	return -1;
}

int IOWriteFunc(void *data, uint8_t *buf, int buf_size)
{
	return -1;
}

CustomIOContext::CustomIOContext() {
	_bufferMutex = SDL_CreateMutex();
	_videoBuffer = nullptr;
	_resetAudio = false;
	_videoBufferSize = 0;
	_bufferSize = 16384;
	_pos = 0;
	_buffer = (uint8_t*)av_malloc(_bufferSize);
	_ioCtx = nullptr;
	_ioCtx = avio_alloc_context(
				_buffer, _bufferSize,
				0,
				(void*)this,
				IOReadFunc, 
				0,
				IOSeekFunc
	);
}

CustomIOContext::~CustomIOContext()
{
	av_free(_buffer);
	av_free(_videoBuffer);
	av_free(_ioCtx);
}

void CustomIOContext::initAVFormatContext(AVFormatContext *pCtx) {
	pCtx->pb = _ioCtx;
	pCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
	
	AVProbeData probeData;
	probeData.buf = _buffer;
	probeData.buf_size = _bufferSize - 1;
	probeData.filename = "";
	pCtx->iformat = av_probe_input_format(&probeData, 1);
}