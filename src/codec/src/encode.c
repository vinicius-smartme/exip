/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file encode.c
 * @brief An EXI encoder
 */

#include "encode.h"
#include "codec_common.h"
#include "EXISerializer.h"
#include "stringManipulate.h"
#include "parseSchema.h"
#include <stdio.h>
#include <string.h>
#include "../../grammarGen/include/grammarGenerator.h"
#include "headerEncode.h"

#define OUTPUT_BUFFER_SIZE 200
#define MAX_ATTRIBUTE_LENGTH 64

const String NS_STR = {"http://www.ltu.se/EISLAB/schema-test", 36};
const String NS_NESTED_STR = {"http://www.ltu.se/EISLAB/nested-xsd", 35};
const String NS_TYPES_STR = {"http://www.ltu.se/EISLAB/types", 30};
const String NS_EMPTY_STR = {NULL, 0};

const String ELEM_ENCODE_STR = {"EXIPEncoder", 11};
const String ELEM_MULT_TEST_STR = {"MultipleXSDsTest", 16};
const String ELEM_DESCR_STR = {"description", 11};
const String ELEM_TYPE_TEST_STR = {"type-test", 9};
const String ELEM_TEST_SETUP_STR = {"testSetup", 9};
const String ELEM_BOOL_STR = {"bool", 4};
const String ELEM_INT_STR = {"int", 3};
const String ELEM_EXT_TYPES_STR = {"extendedTypeTest", 16};
const String ELEM_BYTE_TYPES_STR = {"byteTest", 8};
const String ELEM_DATE_TYPES_STR = {"dateTimeTest", 12};
const String ELEM_BIN_TYPES_STR = {"binaryTest", 10};
const String ELEM_ENUM_TYPES_STR = {"enumTest", 8};

const String ATTR_BYTE_STR = {"testByte", 8};
const String ATTR_VERSION_STR = {"version", 7};
const String ATTR_GOAL_STR = {"goal", 4};
const String ATTR_ID_STR = {"id", 2};

// static char SOME_BINARY_DATA[] = {0x02, 0x6d, 0x2f, 0xa5, 0x20, 0xf2, 0x61, 0x9c, 0xee, 0x0f};
// static String SOME_BINARY_DATA_BASE64 = {"i3sd7fatzxad", 12};
// static String ENUM_DATA_1 = {"hello", 5};
// static String ENUM_DATA_2 = {"hi", 2};
// static String ENUM_DATA_3 = {"hey", 3};
// static String ENUM_DATA_4 = {"hej", 3};

#define TRY_CATCH_ENCODE(func) TRY_CATCH(func, serialize.closeEXIStream(&testStrm))

errorCode read_startDocument(unsigned char inFlag, const char *data)
{
	if (inFlag == IN_EXI)
		return strstr(data, "SD") != NULL ? EXIP_OK : EXIP_INVALID_INPUT;
	else if (inFlag == IN_XML)
		return strcmp(data,  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") == 0 ? EXIP_OK : EXIP_INVALID_INPUT;
	else 
		return EXIP_INVALID_INPUT;
}

errorCode read_endDocument(unsigned char inFlag, const char *data)
{
	if (!data) {
		return EXIP_INVALID_INPUT;
	}

	if (inFlag == IN_EXI)
		return strstr(data, "ED") != NULL ? EXIP_OK : EXIP_INVALID_INPUT;
	else if (inFlag == IN_XML)
		return EXIP_OK;
	else 
		return EXIP_INVALID_INPUT;
}

errorCode read_startElement(unsigned char inFlag, const char *data, String *uri, String *localName)
{
	char* strPtr = NULL;
	char* localNamePtr = NULL;
	size_t strLen = 0;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	char tmp_uri[MAX_ATTRIBUTE_LENGTH];
	char tmp_localName[MAX_ATTRIBUTE_LENGTH];
	memset(tmp_uri, 0, MAX_ATTRIBUTE_LENGTH);
	memset(tmp_localName, 0, MAX_ATTRIBUTE_LENGTH);

	// Clears output parameters
	if (!isStringEmpty(uri)) {
		clearString(uri);
	}
	if (!isStringEmpty(localName)) {
		clearString(localName);
	}

	if (inFlag== IN_EXI)
	{
		strPtr = strstr(data, "SE ");
		if (strPtr == NULL) {
			return EXIP_INVALID_INPUT;
		}
		else {
			strPtr += 3;
		}

		// Checks if there is some Uri element
		localNamePtr = strstr(strPtr, " ");
		if (localNamePtr == NULL) {
			// No Uri
			strLen = strlen(data) - strLen;
			TRY(asciiToString(strPtr, localName, TRUE));
		}
		else {
			// Has Uri
			localNamePtr++; // Remove the space
			strLen = strlen(strPtr) - strlen(localNamePtr);
			strncpy(tmp_uri, strPtr, strLen);
			strLen = strlen(localNamePtr);
			strncpy(tmp_localName, localNamePtr, strLen);
			TRY(asciiToString(tmp_uri, uri, TRUE));
			TRY(asciiToString(tmp_localName, localName, TRUE));
		}
	}
	else if (inFlag== IN_XML)
	{
		strPtr = strstr(data, "<");
		if (strPtr == NULL) {
			return EXIP_INVALID_INPUT;
		}
		else {
			strPtr++;
		}

		// Checks if there is some Uri element
		localNamePtr = strstr(strPtr, ":");
		if (localNamePtr == NULL) {
			// No Uri
			strLen = strlen(data) - strLen;
			TRY(asciiToString(strPtr, localName, TRUE));
		}
		else {
			// Has Uri
			localNamePtr++; // Remove the double dots
			strLen = strlen(strPtr) - strlen(localNamePtr);
			strncpy(tmp_uri, strPtr, strLen);
			strLen = strlen(localNamePtr);
			strncpy(tmp_localName, localNamePtr, strLen);
			TRY(asciiToString(tmp_uri, uri, TRUE));
			TRY(asciiToString(tmp_localName, localName, TRUE));
		}
	}
	else 
		return EXIP_INVALID_INPUT;

	return EXIP_OK;
}

errorCode read_endElement(unsigned char inFlag, const char *data)
{
	char *strPtr = NULL;

	if (inFlag == IN_EXI)
	{
		return strstr(data, "EE") != NULL ? EXIP_OK : EXIP_INVALID_INPUT;
	}
	else if (inFlag == IN_XML)
	{
		strPtr = strstr(data, "</");
		if (strPtr != NULL)
		{
			strPtr += 2;
			return EXIP_OK;
		}
		else 
		{
			return EXIP_INVALID_EVENT;
		}
	}
	else
	{
		return EXIP_INVALID_INPUT;
	}
}

errorCode read_attribute(unsigned char inFlag, const char *data, String *uri, String *localName, EXITypeClass *valueType)
{
	char* strPtr = NULL;
	char* localNamePtr = NULL;
	char* attPtr = NULL;
	char attribute[MAX_ATTRIBUTE_LENGTH];
	char tmp_uri[MAX_ATTRIBUTE_LENGTH];
	char tmp_localName[MAX_ATTRIBUTE_LENGTH];
	memset(attribute, 0, MAX_ATTRIBUTE_LENGTH);
	memset(tmp_uri, 0, MAX_ATTRIBUTE_LENGTH);
	memset(tmp_localName, 0, MAX_ATTRIBUTE_LENGTH);

	size_t strLen = 0;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (!data) 
	{
		// Some parameter is uninitialized
		return EXIP_INVALID_INPUT;
	}
	
	// Clears output parameters
	if (uri && !isStringEmpty(uri)) {
		clearString(uri);
	}
	if (localName && !isStringEmpty(localName)) {
		clearString(localName);
	}

	if (inFlag== IN_EXI)
	{
		strPtr = strstr(data, "AT ");
		if (strPtr == NULL) {
			return EXIP_INVALID_INPUT;
		}
		else {
			strPtr += 3;
		}

		attPtr = strstr(data, "=\"");
		if (attPtr == NULL) 
		{
			// No Attribute found
			return EXIP_INVALID_INPUT;
		}
		else
		{
			attPtr += 2;
			strcpy(attribute, attPtr);
			strLen = strlen(attribute);
			// Remove the quotes
			attribute[strLen-1] = '\0';
		}

		// Checks if there is some Uri element
		localNamePtr = strstr(strPtr, " ");
		if (localNamePtr == NULL) 
		{
			// No Uri
			strLen = strlen(strPtr) - strlen(attPtr) - 2; // Removes the `=` and the quote
			strncpy(tmp_localName, strPtr, strLen);
			TRY(asciiToString(tmp_localName, localName, TRUE));
		}
		else 
		{
			// Has Uri
			localNamePtr++; // Remove the space
			strLen = strlen(strPtr) - strlen(localNamePtr);
			strncpy(tmp_uri, strPtr, strLen);
			strLen = strlen(localNamePtr) - strlen(attPtr) - 2; // Removes the `=` and the quote
			strncpy(tmp_localName, localNamePtr, strLen);
			TRY(asciiToString(tmp_uri, uri, TRUE));
			TRY(asciiToString(tmp_localName, localName, TRUE));
		}
	}
	else if (inFlag== IN_XML)
	{
		strPtr = strstr(data, "<");
		if (strPtr == NULL) {
			return EXIP_INVALID_INPUT;
		}
		else {
			strPtr++;
		}

		attPtr = strstr(data, "=\"");
		if (attPtr == NULL) 
		{
			// No Attribute found
			return EXIP_INVALID_INPUT;
		}
		else
		{
			attPtr += 2;
			strcpy(attribute, attPtr);
			strLen = strlen(attribute);
			// Remove the quotes
			attribute[strLen-1] = '\0';
		}

		// Checks if there is some Uri element
		localNamePtr = strstr(data, ":");
		if (localNamePtr == NULL) {
			// No Uri
			strLen = strlen(data) - strLen;
			TRY(asciiToString(strPtr, localName, TRUE));
		}
		else {
			// Has Uri
			localNamePtr++; // Remove the space
			strLen = strlen(data) - strlen(localNamePtr);
			strncpy(tmp_uri, strPtr, strLen);
			strLen = strlen(localNamePtr) - strlen(attPtr) - 2; // Removes the `=` and the quote
			strncpy(tmp_localName, localNamePtr, strLen);
			TRY(asciiToString(tmp_uri, uri, TRUE));
			TRY(asciiToString(tmp_localName, localName, TRUE));
		}
	}
	else 
		return EXIP_INVALID_INPUT;

	return EXIP_OK;
}

errorCode read_stringData(unsigned char inFlag, const char *data, String *chVal) {
	char* strPtr = NULL;
	size_t strLen = 0;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	// Clears output parameters
	if (!isStringEmpty(chVal)) {
		clearString(chVal);
	}

	if (inFlag== IN_EXI)
	{
		strPtr = strstr(data, "CH ");
		if (strPtr == NULL) {
			return EXIP_INVALID_INPUT;
		}
		else {
			strPtr += 3;
		}

		strLen = strlen(data) - strLen;
		TRY(asciiToString(strPtr, chVal, TRUE));
	}
	else if (inFlag== IN_XML)
	{
		strLen = strlen(data) - strLen;
		TRY(asciiToString(strPtr, chVal, TRUE));
	}
	else 
		return EXIP_INVALID_INPUT;

	return EXIP_OK;
}

errorCode read_namespaceDeclaration(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode read_comment(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode read_processingInstruction(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode read_docType(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode read_EntityReference(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode read_selfContained(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
}

static errorCode encode(
	EXIPSchema *schemaPtr,
	unsigned char inFlag,
	boolean has_options,
	EXIOptions *opts,
	List *inData,
	size_t inDataLen,
	void *outStreamPath,
	size_t (*outputStream)(void *buf, size_t size, void *stream),
	char **outData,
	size_t *outDataLen
	)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri = EMPTY_STRING;
	String ln = EMPTY_STRING;
	QName qname = {&uri, &ln, NULL};
	String chVal = EMPTY_STRING;
	char buf[OUTPUT_BUFFER_SIZE];
	BinaryBuffer buffer;
	EXITypeClass valueType;
	size_t listIdx = 0;
	Node *entry;

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header. Cookie is always TRUE.
	testStrm.header.has_cookie = TRUE;
	testStrm.header.has_options = has_options;
	if (has_options && opts != NULL)
		testStrm.header.opts = *opts;

	// III: Define an external stream for the output if any, otherwise set to NULL
	buffer.ioStrm.readWriteToStream = outputStream;
	buffer.ioStrm.stream = outStreamPath;
	if (outputStream == NULL)
	{
		buffer.bufStrm.buf = calloc(1, OUTPUT_BUFFER_SIZE);
		buffer.bufStrm.bufContent = 0;
		buffer.bufStrm.bufLen = OUTPUT_BUFFER_SIZE;
	}
	
	// IV: Initialize the stream
	TRY_CATCH_ENCODE(serialize.initStream(&testStrm, buffer, schemaPtr));

	// V: Start building the stream step by step: header, document, element etc...
	TRY_CATCH_ENCODE(serialize.exiHeader(&testStrm));

	while(listIdx < inData->size) {
		entry = getNth(inData, listIdx);
		if(listIdx == 0 && read_startDocument(inFlag, (const char *)entry->data) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.startDocument(&testStrm));
			listIdx++;
		}
		else if(read_endDocument(inFlag, (const char *)entry->data) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.endDocument(&testStrm));
			*outDataLen = testStrm.buffer.bufStrm.bufContent;
			*outData = testStrm.buffer.bufStrm.buf;
			TRY_CATCH_ENCODE(serialize.closeEXIStream(&testStrm));
			tmp_err_code = EXIP_OK;
			break;
		}
		else if(read_startElement(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType));
			listIdx++;
		}
		else if(read_endElement(inFlag, (const char *)entry->data) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.endElement(&testStrm));
			listIdx++;
		}
		else if(read_attribute(inFlag, entry->data, &uri, &ln, &valueType) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType));
			listIdx++;
		}
		else if(read_stringData(inFlag, entry->data, &chVal) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
			listIdx++;
		}
		else if(read_namespaceDeclaration(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			//TRY_CATCH_ENCODE(serialize.namespaceDeclaration(&testStrm, chVal));
			listIdx++;
		}
		else if(read_comment(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.comment(&testStrm, chVal));
			listIdx++;
		}
		else if(read_processingInstruction(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			//TRY_CATCH_ENCODE(serialize.processingInstruction(&testStrm));
		}
		else if(read_docType(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			//TRY_CATCH_ENCODE(serialize.docType(&testStrm, chVal));
			listIdx++;
		}
		else if(read_EntityReference(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			//TRY_CATCH_ENCODE(serialize.EntityReference(&testStrm));
			listIdx++;
		}
		else if(read_selfContained(inFlag, entry->data, &uri, &ln) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.selfContained(&testStrm));
			listIdx++;
		}
		else {
			*outDataLen = 0;
			tmp_err_code = EXIP_UNEXPECTED_ERROR;
			break;
		}
	};

	clearString(&chVal);
	
	if(tmp_err_code != EXIP_OK)
	{
		free(buffer.bufStrm.buf);
		closeStream(&testStrm);
		*outDataLen = 0;
	}
	return tmp_err_code;
}

errorCode encodeFromFile(
    char *schemaPath, 
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	const char *inputFilePath, 
	char **outData,
	size_t *outDataLen)
{
	// EXIPSchema schema;
	// EXIPSchema* schemaPtr = NULL;
	// errorCode ret;

	// if (schemaPath)
	// {
	// 	if ((parseSchema(schemaPath, NULL, &schema) != EXIP_OK))
	// 	{
	// 		fprintf(stderr, "Unable to parse schema\n");
	// 		return EXIP_INVALID_INPUT;
	// 	}
	// 	else {
	// 		schemaPtr = &schema;
	// 	}
	// }
	// //return encode(schemaPtr, out_stream, writeFileOutputStream);

	// destroySchema(&schema);
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode encodeFromBuffer(
    char *schemaPath,
	unsigned char outFlag, 
	boolean hasOptions, 
	EXIOptions *options,
	List *inData,
	size_t inDataLen,
	char **outData,
	size_t *outDataLen)
{
	EXIPSchema schema;
	EXIPSchema* schemaPtr = NULL;
	errorCode ret;

	if (schemaPath)
	{
		if ((parseSchema(schemaPath, NULL, &schema) != EXIP_OK))
		{
			fprintf(stderr, "Unable to parse schema\n");
			return EXIP_INVALID_INPUT;
		}
		else {
			schemaPtr = &schema;
		}
	}

	ret = encode(
		schemaPtr,
		outFlag,
		hasOptions,
		options,
		inData,
		inDataLen,
		NULL,
		NULL,
		&(*outData),
		outDataLen
	);

	destroySchema(&schema);

	return ret;
}