#include "headers/CustomIOContext.h"
extern "C"
{
	#include "libavutil/error.h"
}
#include <iostream>

static int IOReadFunc(void *data, uint8_t *buf, int buf_size) {
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

static int64_t IOSeekFunc(void *data, int64_t offset, int whence) {
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
			hctx->_pos += offset;
			return hctx->_pos;
			/*int64_t offsetToIframe = 0;
			bool foundIFrame = false;
			bool gaveUp = false;
			do
			{
				if(isH264iFrame(hctx->_videoBuffer + hctx->_pos + offset + offsetToIframe))
				{
					foundIFrame = true;
				}
				if(!foundIFrame)
				{
					offsetToIframe++;
					if((hctx->_pos + offset + offsetToIframe) > (hctx->_bufferSize - 1))
					{
						gaveUp = true;
					}
				}
			}
			while(!foundIFrame || gaveUp);
			if(foundIFrame)
			{
				if(hctx->_pos + offset > hctx->_bufferSize)
				{
					hctx->_pos = hctx->_bufferSize;
					return -1;
				}
				offset += offsetToIframe;
				std::cout << "off to iFrame:" << offsetToIframe << std::endl;
				hctx->_pos += offset;
				return hctx->_pos;
			}*/
		}
	}
	return -1;
	//TODO: Rewrite after changing buffers
	/*CustomIOContext *hctx = (CustomIOContext*)data;
	if(hctx->_ioCtx == nullptr)
	{
		std::cout << "_ioCtx nullptr" << std::endl;
		return -1;
	}
	if(whence == SEEK_CUR)
	{
		if(hctx->_pos + offset > hctx->_bufferSize)
		{
			hctx->_pos = hctx->_bufferSize;
			return hctx->_bufferSize;
		}
		else if(hctx->_pos + offset < 0)
		{
			hctx->_pos = 0;
			return 0;
		}
		else
		{
			//TODO: Clean this up. Segfaults
			int64_t offsetToIframe = 0;
			bool foundIFrame = false;
			bool gaveUp = false;
			do
			{
				if(isH264iFrame(hctx->_ioCtx->buffer + hctx->_pos + offset + offsetToIframe))
				{
					foundIFrame = true;
				}
				if(!foundIFrame)
				{
					offsetToIframe++;
					if((hctx->_pos + offset + offsetToIframe) < 0 || (hctx->_pos + offset + offsetToIframe) > (hctx->_bufferSize - 1))
					{
						gaveUp = true;
					}
				}
			}
			while(!foundIFrame || gaveUp);
			if(foundIFrame)
			{
				if(hctx->_pos + offset > hctx->_bufferSize)
				{
					hctx->_pos = hctx->_bufferSize;
					return -1;
				}
				std::cout << "offset:" << offset << std::endl;
				std::cout << "whence:" << whence << std::endl;
				offset += offsetToIframe;
				std::cout << "off to iFrame:" << offsetToIframe << std::endl;
				return avio_seek(hctx->_ioCtx, offset, whence);
			}
			//std::cout << "offset:" << offset << std::endl;
			//std::cout << "whence:" << whence << std::endl;
			int64_t status = avio_seek(hctx->_ioCtx, offset, whence);
			return status;
		}
		return -1;
	}
	else
	{
		hctx->_pos += offset;
		return hctx->_pos;
	}*/
}

static int IOWriteFunc(void *data, uint8_t *buf, int buf_size)
{
	return -1;
	//Useless
	/*CustomIOContext *hctx = static_cast<CustomIOContext*>(data);
	uint8_t* helpBuffer = (uint8_t*)av_malloc(hctx->_bufferSize + buf_size);
	memcpy(helpBuffer, hctx->_buffer, hctx->_bufferSize);
	memcpy(helpBuffer, buf + hctx->_bufferSize, buf_size);
	av_free(hctx->_buffer);
	hctx->_buffer = helpBuffer;
	return buf_size;*/
}

CustomIOContext::CustomIOContext() {
	_videoBuffer = nullptr;
	_resetAudio = false;
	_videoBufferSize = 0;
	_bufferSize = 16384;
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