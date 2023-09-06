/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file ioUtil.h
 * @brief Common utilities for StreamIO module
 *
 * @date Oct 26, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#ifndef IOUTIL_H_
#define IOUTIL_H_

#include "procTypes.h"
#include "errorHandle.h"

/**
 * @brief Moves the BitPointer with certain positions. Takes care of byteIndex increasing when
 *        the movement cross a byte boundary
 * @param[in] strm EXI stream of bits
 * @param[in] bitPositions the number of bit positions to move the pointer
 */
void moveBitPointer(EXIStream* strm, unsigned int bitPositions);

/**
 * @brief Determine the number of bits needed to encode an unsigned integer value
 * ⌈ log 2 m ⌉ from the spec is equal to getBitsNumber(m - 1)
 *
 * @param[in] val unsigned integer value
 *
 * @return The number of bits needed
 */
unsigned char getBitsNumber(uint64_t val);

/**
 * @brief Log2 function. Used to determine the number of bits needed to encode a unsigned integer value
 * The code taken from: http://www-graphics.stanford.edu/~seander/bithacks.html#IntegerLog
 * @param[in] val uint32_t value
 *
 * @return The number of bits needed
 */
unsigned int log2INT(uint64_t val);

/**
 * @brief Reads an EXI stream chunk using buffer.ioStrm.readWriteToStream if available
 * @param[in] strm EXI stream of bits
 * @param[in] numBytesToBeRead the number of bytes that are requested for parsing
 *
 * @return The number of bits needed
 */
errorCode readEXIChunkForParsing(EXIStream* strm, unsigned int numBytesToBeRead);

/**
 * @brief Flushes the EXI buffer using buffer.ioStrm.readWriteToStream if available
 * @param[in] strm EXI stream of bits
 *
 * @return The number of bits needed
 */
errorCode writeEncodedEXIChunk(EXIStream* strm);


/**
 * @brief Reads the available data from the Stream(IO or buffer) into the memory buffer, if any
 * @note A function pointer used to fill the EXI buffer when emptied by reading "doSize" number of bytes.
 * @note Offset will be applied to the internal buffer pointer and not to the stream/external buffer.
 * @param[in] buffer BinaryBuffer to operate the read
 * @param[in] offset offsets the destination buffer address
 * @param[in] doSize number of bytes to read from the stream
 * @param[in] doneSize number of bytes to write to the buffer
 *
 * @return The error code
 */
errorCode readFromStream(BinaryBuffer* buffer, Index offset, size_t doSize, Index* doneSize) ;

/**
 * @brief Writes the available data to the  Stream(IO or buffer) from the memory buffer, if any
 * @note A function pointer used to write "doSize" number of bytes from the buffer. If the buffer size is insufficient,
 * it tries to reallocate memory to fit more data.
 * @note Offset will be applied to the internal buffer pointer and not to the stream/external buffer.
 * @param[in] buffer BinaryBuffer to operate the write
 * @param[in] offset offsets the destination buffer address
 * @param[in] doSize number of bytes to read from the buffer
 * @param[in] doneSize number of bytes to written to the stream
 *
 * @return The error code
 */
errorCode writeToStream(BinaryBuffer* buffer, Index offset, size_t doSize, Index* doneSize) ;
#endif /* IOUTIL_H_ */
