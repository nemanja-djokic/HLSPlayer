#include "headers/CustomIOContext.h"
extern "C"
{
	#include "libavutil/error.h"
}
#include <iostream>

int IOReadFunc(void *data, uint8_t *buf, int buf_size)
{
	CustomIOContext *hctx = (CustomIOContext*)data;
	if(hctx->_block == 0 && hctx->_pos == 0)
	{
		PlaylistSegment* currentSegment = hctx->_networkManager->getSegment(0);
		currentSegment->loadSegment();
	}
	if(hctx->_block >= (int32_t)hctx->_networkManager->getSegmentsSize() - 1)
	{
		return AVERROR_EOF;
	}
	SDL_LockMutex(hctx->_bufferMutex);
	int64_t pos = hctx->_pos;
	PlaylistSegment* currentSegment = hctx->_networkManager->getSegment(hctx->_block);
	if(currentSegment == nullptr)
	{
		return 0;
	}
	if(!currentSegment->getIsLoaded())
	{
		hctx->_networkManager->updateCurrentSegment(hctx->_block);
		SDL_SemPost(hctx->_networkManager->getBlockEndSemaphore());
		SDL_SemPost(hctx->_networkManager->getBlockEndSemaphore());
		std::cerr << "Loading in read callback!" << std::endl;
		while(!currentSegment->getIsLoaded())currentSegment->loadSegment();
	}
	if(pos + buf_size <= (int)currentSegment->loadedSize())
	{
		if(hctx->_pos == 0 && hctx->_networkManager->isBitrateDiscontinuity())
		{
			std::cout << "Discontinuity reset" << std::endl;
			hctx->_resetAudio = true;
			avio_flush(hctx->_ioCtx);
			avformat_flush(hctx->_formatContext);
			if(hctx->_videoCodec != nullptr)
        	{
            	avcodec_flush_buffers(hctx->_videoCodec);
        	}
			if(hctx->_audioCodec != nullptr)
        	{
            	avcodec_flush_buffers(hctx->_audioCodec);
        	}
			hctx->_networkManager->clearBitrateDiscontinuity();
		}
		memcpy(buf, currentSegment->getTsData() + pos, buf_size);
		hctx->_pos += buf_size;
		SDL_UnlockMutex(hctx->_bufferMutex);
		return buf_size;
	}
	else
	{
		if(hctx->_pos == 0 && hctx->_networkManager->isBitrateDiscontinuity())
		{
			std::cout << "Discontinuity reset" << std::endl;
			hctx->_resetAudio = true;
			avio_flush(hctx->_ioCtx);
			avformat_flush(hctx->_formatContext);
			if(hctx->_videoCodec != nullptr)
        	{
            	avcodec_flush_buffers(hctx->_videoCodec);
        	}
			if(hctx->_audioCodec != nullptr)
        	{
            	avcodec_flush_buffers(hctx->_audioCodec);
        	}
			hctx->_networkManager->clearBitrateDiscontinuity();
		}
		memcpy(buf, currentSegment->getTsData() + pos, currentSegment->loadedSize() - pos);
		hctx->_pos = 0;
		hctx->_block++;
		hctx->_networkManager->updateCurrentSegment(hctx->_block);
		SDL_SemPost(hctx->_networkManager->getBlockEndSemaphore());
		SDL_SemPost(hctx->_networkManager->getBlockEndSemaphore());
		SDL_UnlockMutex(hctx->_bufferMutex);
		return currentSegment->loadedSize() - pos;
	}
	SDL_UnlockMutex(hctx->_bufferMutex);
	return 0;
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
		hctx->_block = hctx->_blockToSeek;
		hctx->_pos = 0;
		hctx->_networkManager->updateCurrentSegment(hctx->_block);
		SDL_SemPost(hctx->_networkManager->getBlockEndSemaphore());
		SDL_UnlockMutex(hctx->_bufferMutex);
		return hctx->_block * hctx->_pos;
	}
	SDL_UnlockMutex(hctx->_bufferMutex);
	return 0;
}

int IOWriteFunc(void *data, uint8_t *buf, int buf_size)
{
	return -1;
}

CustomIOContext::CustomIOContext() {
	_bufferMutex = SDL_CreateMutex();
	_resetAudio = false;
	_bufferSize = 16384;
	_pos = 0;
	_block = 0;
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