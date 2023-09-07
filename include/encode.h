/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file encode.h
 * @brief Interface for a function encoding a sample EXI message
 */

#ifndef ENCODE_H_
#define ENCODE_H_

#include "EXIOptions.h"
#include "EXIPrimitives.h"
#include "singleLinkedList.h"
#include "errorHandle.h"

#define IN_EXI 0
#define IN_XML 1

errorCode encodeFromFile(
    char *schemaPath, 
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	const char *inputFilePath, 
	char **outData,
	size_t *outDataLen);

errorCode encodeFromBuffer(
    char *schemaPath,
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	List *inData,
	size_t inDataLen,
	char **outData,
	size_t *outDataLen);

#endif /* ENCODE_H_ */