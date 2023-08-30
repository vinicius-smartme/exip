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

#include "procTypes.h"
#include "codec_utils.h"

#define IN_EXI 0
#define IN_XML 1

errorCode encode_from_file(
    EXIPSchema *schemaPtr, 
	unsigned char outFlag, 
	boolean outOfBandOpts, 
	EXIOptions *opts,
	void *inStreamPath, 
	List *outData);

errorCode encode_from_buffer(
    EXIPSchema *schemaPtr, 
	unsigned char outFlag, 
	boolean has_options, 
	EXIOptions *opts,
	List *inData,
	size_t inDataLen,
	void *outData,
	size_t outDataLen);


errorCode read_startDocument(unsigned char inFlag, const char *data);
errorCode read_endDocument(unsigned char inFlag, const char *data);
errorCode read_startElement(unsigned char inFlag, const char *data, List *elementList, String *uri, String *localName);
errorCode read_endElement(unsigned char inFlag, const char *data, List *elementList);
errorCode read_attribute(unsigned char inFlag, const char *data, String *uri, String *localName, EXITypeClass *valueType);
errorCode read_stringData(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_namespaceDeclaration(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_comment(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_processingInstruction(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_docType(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_EntityReference(unsigned char inFlag, const char *data, String *uri, String *localName);
errorCode read_selfContained(unsigned char inFlag, const char *data, String *uri, String *localName);
#endif /* ENCODE_H_ */