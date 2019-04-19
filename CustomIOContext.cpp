#include "headers/CustomIOContext.h"
extern "C"
{
	#include "libavutil/error.h"
}
#include <iostream>

static int IOReadFunc(void *data, uint8_t *buf, int buf_size)
{
	CustomIOContext *hctx = (CustomIOContext*)data;
	if (hctx->_pos >= hctx->_videoBufferSize)
    {
		return AVERROR_EOF;
	}
	int32_t toRead = (buf_size < 16384)?buf_size:16384;
	toRead = (toRead < (hctx->_videoBufferSize - hctx->_pos))?toRead:(hctx->_videoBufferSize - hctx->_pos);
	if(toRead <= 0)
		return AVERROR_EOF;
	
	memcpy(buf, hctx->_videoBuffer + hctx->_pos, toRead);
	hctx->_pos += toRead;
	return toRead;
}

static bool isH264iFrame(uint8_t* paket)
{
	int RTPHeaderBytes = 0;
	int fragment_type = paket[RTPHeaderBytes + 0] & 0x1F;
	int nal_type = paket[RTPHeaderBytes + 1] & 0x1F;
	int start_bit = paket[RTPHeaderBytes + 1] & 0x80;
	if (((fragment_type == 28 || fragment_type == 29) && nal_type == 5 && start_bit == 128) || fragment_type == 5)
    {
        return true;
    }

    return false;
}

static int64_t IOSeekFunc(void *data, int64_t offset, int whence)
{
	CustomIOContext* hctx = (CustomIOContext*)data;
	if(hctx->_ioCtx == nullptr)
	{
		std::cerr << "IO Context Missing" << std::endl;
		return -1;
	}
	if(whence == SEEK_CUR)
	{
		hctx->_resetAudio = true;
		if(hctx->_pos + offset > hctx->_videoBufferSize - 1)
		{
			hctx->_pos = hctx->_videoBufferSize - 1;
			return hctx->_videoBufferSize - 1;
		}
		else if(hctx->_pos + offset < 0)
		{
			hctx->_pos = 0;
			return 0;
		}
		else
		{
			//TODO: Not working, maybe fix later
			int64_t offsetToIframe = 0;
			int64_t beginningOfTsBlock = -1;
			bool foundIFrame = false;
			bool gaveUp = false;
			do
			{
				if(hctx->_videoBuffer[hctx->_pos + offset + offsetToIframe] == 0x47)
				{
					std::cout << "reading ts" << std::endl;
					beginningOfTsBlock = hctx->_pos + offset + offsetToIframe;
				}
				if(beginningOfTsBlock != -1 && isH264iFrame(hctx->_videoBuffer + hctx->_pos + offset + offsetToIframe))
				{
					std::cout << "found iframe" << std::endl;
					foundIFrame = true;
				}
				if(!foundIFrame)
				{
					if(offset > 0)
					{
						offsetToIframe++;
					}
					else
					{
						offsetToIframe--;
					}
					if((hctx->_pos + offset + offsetToIframe) > (hctx->_videoBufferSize - 1))
					{
						gaveUp = true;
					}
					else if((hctx->_pos + offset + offsetToIframe) < 0)
					{
						hctx->_pos = 0;
						return 0;
					}
				}
			}
			while(!foundIFrame || gaveUp);
			if(foundIFrame)
			{
				if(beginningOfTsBlock < 0)
				{
					hctx->_pos = 0;
				}
				else if(beginningOfTsBlock > hctx->_videoBufferSize)
				{
					hctx->_pos = hctx->_videoBufferSize;
				}
				else
				{
					hctx->_pos = beginningOfTsBlock;
					avio_flush(hctx->_ioCtx);
				}
				return hctx->_pos;
			}
		}
	}
	else if(whence == SEEK_SET)
	{
		if(offset < 0)
		{
			hctx->_pos = 0;
			return 0;
		}
		else if(offset > hctx->_videoBufferSize)
		{
			hctx->_pos = hctx->_videoBufferSize;
			return hctx->_pos;
		}
		else
		{
			hctx->_pos = offset;
			return offset;
		}
		
	}
	return -1;
}

static int IOWriteFunc(void *data, uint8_t *buf, int buf_size)
{
	return -1;
}

int AudioReadFunc(void *data, uint8_t *buf, int buf_size)
{
	CustomIOContext *hctx = (CustomIOContext*)data;
	if(buf_size == 0)
	{
		return 0;
	}
	if (hctx->_audioPos >= hctx->_audioBufferSize)
	{
		return AVERROR_EOF;
	}
	int32_t toRead = buf_size;
	toRead = (toRead < (hctx->_audioBufferSize - hctx->_audioPos))?toRead:(hctx->_audioBufferSize - hctx->_audioPos);
	if(toRead <= 0)
		return 0;

	memset(buf, 0, buf_size);
	SDL_MixAudio(buf, hctx->_audioBuffer + hctx->_audioPos, buf_size, SDL_MIX_MAXVOLUME);
	//memcpy(buf, hctx->_audioBuffer + hctx->_audioPos, toRead);
	hctx->_audioPos += toRead;
	return toRead;
}

CustomIOContext::CustomIOContext() {
	_videoBuffer = nullptr;
	_audioBuffer = nullptr;
	_resetAudio = true;
	_videoBufferSize = 0;
	_audioBufferSize = 0;
	_bufferSize = 16384;
	_pos = 0;
	_audioPos = 0;
	_audioPts = 0.0;
	_videoPts = 0.0;
	_syncOffset = 0;
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