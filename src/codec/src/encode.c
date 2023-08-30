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
#include <stdio.h>
#include <string.h>

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

static char SOME_BINARY_DATA[] = {0x02, 0x6d, 0x2f, 0xa5, 0x20, 0xf2, 0x61, 0x9c, 0xee, 0x0f};
static String SOME_BINARY_DATA_BASE64 = {"i3sd7fatzxad", 12};
// static String ENUM_DATA_1 = {"hello", 5};
// static String ENUM_DATA_2 = {"hi", 2};
// static String ENUM_DATA_3 = {"hey", 3};
static String ENUM_DATA_4 = {"hej", 3};

#define TRY_CATCH_ENCODE(func) TRY_CATCH(func, serialize.closeEXIStream(&testStrm))

static errorCode encode(
	EXIPSchema *schemaPtr,
	unsigned char inFlag,
	boolean has_options,
	EXIOptions *opts,
	List *inData,
	size_t inDataLen,
	void *outStreamPath,
	size_t (*outputStream)(void *buf, size_t size, void *stream),
	void *outData,
	size_t outDataLen
	)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri = EMPTY_STRING;
	String ln = EMPTY_STRING;
	QName qname = {&uri, &ln, NULL};
	String chVal;
	char buf[OUTPUT_BUFFER_SIZE];
	BinaryBuffer buffer;
	EXITypeClass valueType;
	size_t listIdx = 0;
	Node *entry;
	List elementList = new_list();	// Used to verify that an xml element starts and ends

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
	if (outputStream == NULL && outData != NULL && outDataLen > 0)
	{
		buffer.bufStrm.buf = outData;
		buffer.bufStrm.bufContent = 0;
		buffer.bufStrm.bufLen = outDataLen;
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
			outDataLen = testStrm.buffer.bufContent;
			outData = calloc(1, outDataLen);
			memcpy(outData, testStrm.buffer.buf, outDataLen);
			TRY_CATCH_ENCODE(serialize.closeEXIStream(&testStrm));
			return EXIP_OK;
		}
		else if(read_startElement(inFlag, entry->data, &elementList, &uri, &ln) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType));
			listIdx++;
		}
		else if(read_endElement(inFlag, (const char *)entry->data, &elementList) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.endElement(&testStrm));
			listIdx++;
		}
		else if(read_attribute(inFlag, entry->data, &uri, &ln, &valueType) == EXIP_OK)
		{
			TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType));
			listIdx++;
		}
		else if(read_stringData(inFlag, entry->data, &uri, &ln) == EXIP_OK)
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
			outDataLen = 0;
			return EXIP_UNEXPECTED_ERROR;
		}
	};
	/*
	qname.uri = &NS_STR;
	qname.localName = &ELEM_MULT_TEST_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType));

	qname.uri = &NS_STR;
	qname.localName = &ELEM_ENCODE_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <EXIPEncoder>

	// NOTE: attributes should come lexicographically sorted during serialization

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ATTR_BYTE_STR;

	if (schemaPtr != NULL)
	{
		// schema mode
		TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // testByte="
		TRY_CATCH_ENCODE(serialize.intData(&testStrm, 55));
	}
	else
	{
		// schema-less mode
		TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // testByte="
		TRY_CATCH_ENCODE(asciiToStringManaged("55", &chVal, &testStrm.memList, FALSE));
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
	}

	qname.localName = &ATTR_VERSION_STR;
	TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // version="

	TRY_CATCH_ENCODE(asciiToStringManaged("0.2", &chVal, &testStrm.memList, FALSE));
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));

	TRY_CATCH_ENCODE(asciiToStringManaged("This is an example of serializing EXI streams using EXIP low level API", &chVal, &testStrm.memList, FALSE));
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </EXIPEncoder>

	qname.uri = &NS_STR;
	qname.localName = &ELEM_DESCR_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <description>

	TRY_CATCH_ENCODE(asciiToStringManaged("This is a test of processing XML schemes with multiple XSD files", &chVal, &testStrm.memList, FALSE));
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </description>

	qname.uri = &NS_NESTED_STR;
	qname.localName = &ELEM_TEST_SETUP_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <testSetup>

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ATTR_GOAL_STR;
	TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // goal="

	TRY_CATCH_ENCODE(asciiToStringManaged("Verify that the implementation works!", &chVal, &testStrm.memList, FALSE));
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));

	TRY_CATCH_ENCODE(asciiToStringManaged("Simple test element with single attribute", &chVal, &testStrm.memList, FALSE));
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </testSetup>

	qname.uri = &NS_STR;
	qname.localName = &ELEM_TYPE_TEST_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <type-test>

	if (schemaPtr != NULL)
	{
		// schema mode
		qname.uri = &NS_EMPTY_STR;
		qname.localName = &ATTR_ID_STR;
		TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // id="
		TRY_CATCH_ENCODE(serialize.intData(&testStrm, 1001));
	}
	else
	{
		// schema-less mode
		qname.uri = &NS_EMPTY_STR;
		qname.localName = &ATTR_ID_STR;
		TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // id="
		TRY_CATCH_ENCODE(asciiToStringManaged("1001", &chVal, &testStrm.memList, FALSE));
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
	}

	qname.uri = &NS_NESTED_STR;
	qname.localName = &ELEM_BOOL_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <bool>

	if (schemaPtr != NULL)
	{
		// schema mode
		TRY_CATCH_ENCODE(serialize.booleanData(&testStrm, TRUE));
	}
	else
	{
		// schema-less mode
		TRY_CATCH_ENCODE(asciiToStringManaged("true", &chVal, &testStrm.memList, FALSE));
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
	}

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </bool>

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </type-test>

	qname.uri = &NS_STR;
	qname.localName = &ELEM_EXT_TYPES_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <extendedTypeTest>

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_BYTE_TYPES_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <byteTest>

	if (schemaPtr != NULL)
	{
		// schema mode
		TRY_CATCH_ENCODE(serialize.intData(&testStrm, 11));
	}
	else
	{
		// schema-less mode
		TRY_CATCH_ENCODE(asciiToStringManaged("11", &chVal, &testStrm.memList, FALSE));
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
	}

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </byteTest>

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_DATE_TYPES_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <dateTimeTest>

	if (schemaPtr != NULL)
	{
		// schema mode
		EXIPDateTime dt;

		dt.presenceMask = FRACT_PRESENCE;

		dt.dateTime.tm_year = 112; // 2012
		dt.dateTime.tm_mon = 6;	   // July
		dt.dateTime.tm_mday = 31;
		dt.dateTime.tm_hour = 13;
		dt.dateTime.tm_min = 33;
		dt.dateTime.tm_sec = 55;
		dt.fSecs.value = 839;
		dt.fSecs.offset = 5;

		TRY_CATCH_ENCODE(serialize.dateTimeData(&testStrm, dt));
	}
	else
	{
		// schema-less mode
		TRY_CATCH_ENCODE(asciiToStringManaged("2012 Jul 31 13:33", &chVal, &testStrm.memList, FALSE));
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, chVal));
	}

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </dateTimeTest>

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_BIN_TYPES_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <binaryTest>

	if (schemaPtr != NULL)
	{
		// schema mode
		TRY_CATCH_ENCODE(serialize.binaryData(&testStrm, SOME_BINARY_DATA, 10));
	}
	else
	{
		// schema-less mode
		TRY_CATCH_ENCODE(serialize.stringData(&testStrm, SOME_BINARY_DATA_BASE64));
	}

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </binaryTest>

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_ENUM_TYPES_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <enumTest>
	TRY_CATCH_ENCODE(serialize.stringData(&testStrm, ENUM_DATA_4));
	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </enumTest>

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </extendedTypeTest>

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </MultipleXSDsTest>
	TRY_CATCH_ENCODE(serialize.endDocument(&testStrm));
	
	// VI: Free the memory allocated by the EXI stream object
	TRY_CATCH_ENCODE(serialize.closeEXIStream(&testStrm));

	return tmp_err_code;*/
}

errorCode encode_from_file(
    EXIPSchema *schemaPtr, 
	unsigned char outFlag, 
	boolean has_options, 
	EXIOptions *opts,
	void *outStreamPath, 
	List *outData)
{
	//return encode(schemaPtr, out_stream, writeFileOutputStream);
	return 0;
}

errorCode encode_from_buffer(
    EXIPSchema *schemaPtr, 
	unsigned char outFlag, 
	boolean has_options, 
	EXIOptions *opts,
	List *inData,
	size_t inDataLen,
	void *outData,
	size_t outDataLen)
{
	return encode(
		schemaPtr,
		outFlag,
		has_options,
		opts,
		inData,
		inDataLen,
		NULL,
		NULL,
		outData,
		outDataLen
	);
}

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

errorCode read_startElement(unsigned char inFlag, const char *data, List *elementList, String *uri, String *localName)
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

		// This entry will be verified when ending the element
		push_back(elementList, localName->str, localName->length);
	}
	else 
		return EXIP_INVALID_INPUT;

	return EXIP_OK;
}

errorCode read_endElement(unsigned char inFlag, const char *data, List *elementList)
{
	char *strPtr = NULL;
	Node *entry;
	if (inFlag == IN_EXI)
		return strstr(data, "EE") != NULL ? EXIP_OK : EXIP_INVALID_INPUT;
	else if (inFlag == IN_XML)
	{
		strPtr = strstr(data, "</");
		if (strPtr != NULL)
		{
			strPtr += 2;
			// Verifies if the ending the element matches the one in the list
			entry = pop_back(elementList);
			if (strstr(entry->data, strPtr) != NULL) {
				clear_node(entry);
				return EXIP_OK;
			}
			else
			{
				clear_node(entry);
				return EXIP_INVALID_EVENT;
			}
		    
		}
		else 
			return EXIP_INVALID_INPUT;
	}
	else 
		return EXIP_INVALID_INPUT;
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

errorCode read_stringData(unsigned char inFlag, const char *data, String *uri, String *localName) {
	return EXIP_NOT_IMPLEMENTED_YET;
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