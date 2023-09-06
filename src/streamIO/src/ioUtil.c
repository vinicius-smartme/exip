/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file ioUtil.c
 * @brief Implements common utilities for StreamIO module
 *
 * @date Oct 26, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "ioUtil.h"

void moveBitPointer(EXIStream* strm, unsigned int bitPositions)
{
	int nbits;

	strm->context.bufferIndx += bitPositions/8;
	nbits = bitPositions % 8;
	if(nbits < 8 - strm->context.bitPointer) // The remaining (0-7) bit positions can be moved within the current byte
	{
		strm->context.bitPointer += nbits;
	}
	else
	{
		strm->context.bufferIndx += 1;
		strm->context.bitPointer = nbits - (8 - strm->context.bitPointer);
	}
}

unsigned char getBitsNumber(uint64_t val)
{
	switch(val)
	{
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 2;
		case 4:
			return 3;
		case 5:
			return 3;
		case 6:
			return 3;
		case 7:
			return 3;
		case 8:
			return 4;
		case 9:
			return 4;
		case 10:
			return 4;
		case 11:
			return 4;
		case 12:
			return 4;
		case 13:
			return 4;
		case 14:
			return 4;
		case 15:
			return 4;
		default:
		{
			if(val < 32)
				return 5;
			else
				return log2INT(val) + 1;
		}
	}
}

unsigned int log2INT(uint64_t val)
{
	const uint64_t b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000};
	const unsigned int S[] = {1, 2, 4, 8, 16, 32};
	int i;

	unsigned int r = 0; // result of log2(v) will go here
	for (i = 5; i >= 0; i--) // unroll for speed...
	{
	  if (val & b[i])
	  {
		val >>= S[i];
	    r |= S[i];
	  }
	}
	return r;
}

errorCode readEXIChunkForParsing(EXIStream* strm, unsigned int numBytesToBeRead)
{
	Index bytesCopied = strm->buffer.bufContent - strm->context.bufferIndx;
	Index bytesRead = 0;

	if(strm->buffer.ioStrm.readWriteToStream == NULL && strm->buffer.bufStrm.buf == NULL)
		return EXIP_BUFFER_END_REACHED;

	/* Checks for possible overlaps when copying the left Over Bits,
	 * normally should not happen when the size of strm->buffer is set
	 * reasonably (16 bytes or higher) */
	if(2*bytesCopied > strm->buffer.bufLen)
		return EXIP_INCONSISTENT_PROC_STATE;

	memcpy(strm->buffer.buf, strm->buffer.buf + strm->context.bufferIndx, bytesCopied);

	errorCode error = readFromStream(&(strm->buffer), bytesCopied, strm->buffer.bufLen - bytesCopied, &bytesRead);
	if (error != EXIP_OK)  
		return error;

	strm->buffer.bufContent = bytesCopied + bytesRead;
	if(strm->buffer.bufContent < numBytesToBeRead)
		return EXIP_UNEXPECTED_ERROR;

	strm->context.bufferIndx = 0;

	return EXIP_OK;
}

errorCode writeEncodedEXIChunk(EXIStream* strm)
{
	char leftOverBits;
	Index numBytesWritten = 0;

	if(strm->buffer.ioStrm.readWriteToStream == NULL && strm->buffer.bufStrm.buf == NULL)
		return EXIP_BUFFER_END_REACHED;

	leftOverBits = strm->buffer.buf[strm->context.bufferIndx];

	errorCode error = writeToStream(&(strm->buffer), 0, strm->context.bufferIndx, &numBytesWritten);
	if (error != EXIP_OK)  
		return error;
	if(numBytesWritten < strm->context.bufferIndx)
		return EXIP_UNEXPECTED_ERROR;

	strm->buffer.buf[0] = leftOverBits;
	strm->context.bufferIndx = 0;

	return EXIP_OK;
}

errorCode readFromStream(BinaryBuffer* buffer, Index offset, size_t doSize, Index* doneSize) 
{
	size_t newSize = 0;
	char* tmpPtr = NULL;

	if(buffer->ioStrm.readWriteToStream == NULL)
	{
		if(buffer->bufStrm.bufContent == 0) {
			printf("No Content to read/write \n");
			*doneSize = 0;
			return EXIP_BUFFER_END_REACHED;
		}
		else {
			if (doSize + buffer->bufStrm.bufPtr > buffer->bufStrm.bufContent) {
				printf("doSize outside bounds.");
				*doneSize = buffer->bufStrm.bufContent - buffer->bufStrm.bufPtr;
			} else {
				*doneSize = doSize;
			}
			// Do read
			memcpy(buffer->buf + offset, buffer->bufStrm.buf + buffer->bufStrm.bufPtr, *doneSize);
			buffer->bufContent = offset + *doneSize;
			buffer->bufStrm.bufPtr += *doneSize;
		}
	}
	else {
		*doneSize = (Index)buffer->ioStrm.readWriteToStream(buffer->buf + offset, doSize, buffer->ioStrm.stream);
	}
	return EXIP_OK;
}

errorCode writeToStream(BinaryBuffer* buffer, Index offset, size_t doSize, Index* doneSize) 
{
	size_t newSize = 0;
	char* tmpPtr = NULL;

	if(buffer->ioStrm.readWriteToStream == NULL)
	{
		if (doSize + buffer->bufStrm.bufPtr > buffer->bufStrm.bufLen) {
			// writing outside bounds. Trying to reallocate memory
			// Rounds up a multiple of 2 for the new size
			newSize = 2<<(getBitsNumber(doSize + buffer->bufStrm.bufPtr) - 1);
			int* tmp = (int*)realloc(buffer->bufStrm.buf, newSize);
			if(tmp == NULL)
			{
				// free(buffer->bufStrm.buf);
				fprintf(stderr, "Failed to reallocate memory for the buffer\n");
				return EXIP_MEMORY_ALLOCATION_ERROR;
			}
			else if(buffer->buf != tmp)
			{
				buffer->bufStrm.buf = tmp;
				buffer->bufStrm.bufLen = newSize;
			}
			tmp = NULL;
		}
		*doneSize = doSize;
		// Do write
		memcpy(buffer->bufStrm.buf + buffer->bufStrm.bufPtr, buffer->buf + offset, *doneSize);
		buffer->bufStrm.bufContent += *doneSize;
		buffer->bufStrm.bufPtr += *doneSize;
		// printf("from/to buffer(offset: %d, size: %d, read %d)\n", offset, doSize, *doneSize);
	}
	else {
		*doneSize = (Index)buffer->ioStrm.readWriteToStream(buffer->buf + offset, doSize, buffer->ioStrm.stream);
		// printf("from/to stream(offset: %d, size: %d, read %d)\n", offset, doSize, *doneSize);
	}
	return EXIP_OK;
}