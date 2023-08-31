/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file decode.h
 * @brief Interface for a function decoding sample EXI messages
 */

#ifndef DECODE_H_
#define DECODE_H_

#include "EXIOptions.h"
#include "EXIPrimitives.h"
#include "single_linked_list.h"
#include "errorHandle.h"

#define OUT_EXI 0
#define OUT_XML 1

errorCode decode_from_file(
	const char *schemaPath, 
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	void *inStreamPath, 
	List *outData);

errorCode decode_from_buffer(
	const char *schemaPath, 
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	void *inData, 
	size_t inDataLen, 
	List *outData);

// errorCode decode_from_file(
// 	EXIPSchema *schemaPtr, 
// 	unsigned char outFlag, 
// 	boolean outOfBandOpts, 
// 	EXIOptions *opts,
// 	void *inStreamPath, 
// 	List *outData);

// errorCode decode_from_buffer(
// 	EXIPSchema *schemaPtr, 
// 	unsigned char outFlag, 
// 	boolean outOfBandOpts, 
// 	EXIOptions *opts,
// 	void *inData, 
// 	size_t inDataLen, 
// 	List *outData);

#endif /* DECODE_H_ */