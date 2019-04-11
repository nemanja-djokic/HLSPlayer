#include "headers/CustomIOContext.h"
#include <iostream>

static int IOReadFunc(void *data, uint8_t *buf, int buf_size) {
	CustomIOContext *hctx = (CustomIOContext*)data;
	if (hctx->_pos >= hctx->_bufferSize)
    {
		return AVERROR_EOF;
	}
    size_t toWrite = (buf_size < (hctx->_bufferSize - hctx->_pos))?buf_size:hctx->_bufferSize - hctx->_pos;
    memcpy(buf, hctx->_buffer + hctx->_pos, toWrite);
    hctx->_pos += toWrite;
	return toWrite;
}

static int64_t IOSeekFunc(void *data, int64_t pos, int whence) {
	/*if (whence == AVSEEK_SIZE) {
	}
	
	CustomIOContext *hctx = (CustomIOContext*)data;
	int rs = fseek(hctx->fh, (long)pos, whence);
	if (rs != 0) {
		return -1;
	}
	long fpos = ftell(hctx->fh);
	return (int64_t)fpos;*/
    return 0;
}

static int IOWriteFunc(void *data, uint8_t *buf, int buf_size)
{
	CustomIOContext *hctx = (CustomIOContext*)data;
	uint8_t* helpBuffer = (uint8_t*)av_malloc(hctx->_bufferSize + buf_size);
	memcpy(helpBuffer, hctx->_buffer, hctx->_bufferSize);
	memcpy(helpBuffer, buf + hctx->_bufferSize, buf_size);
	av_free(hctx->_buffer);
	hctx->_buffer = helpBuffer;
	return buf_size;
}

CustomIOContext::CustomIOContext(uint8_t* buffer, size_t size) {
	_bufferSize = size;
	_buffer = new uint8_t[_bufferSize];
	_ioCtx = nullptr;
	//_toDelete = _buffer;
	memcpy(_buffer, buffer, _bufferSize);
	_ioCtx = avio_alloc_context(
				_buffer, _bufferSize,
				0,
				(void*)this,
				IOReadFunc, 
				IOWriteFunc,
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