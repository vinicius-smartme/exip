/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file check_exip.c
 * @brief Tests the whole EXIP library with some test input data
 *
 * @date Nov 4, 2011
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "procTypes.h"
#include "EXISerializer.h"
#include "EXIParser.h"
#include "stringManipulate.h"
#include "grammarGenerator.h"
#include "parseSchema.h"

#define MAX_PATH_LEN 200
#define OUTPUT_BUFFER_SIZE 2000
/* Location for external test data */
static char *dataDir;

static size_t writeFileOutputStream(void* buf, size_t readSize, void* stream);
static size_t readFileInputStream(void* buf, size_t readSize, void* stream);
static void parseMultiSchema(char** xsdList, int count, EXIPSchema* schema);

#define TRY_CATCH_ENCODE(func) TRY_CATCH(func, serialize.closeEXIStream(&testStrm))

/* BEGIN: SchemaLess tests */

START_TEST (test_default_options)
{
	EXIStream testStrm;
	Parser testParser;
	String uri;
	String ln;
	QName qname= {&uri, &ln};
	String chVal;
	char buf[OUTPUT_BUFFER_SIZE];
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	BinaryBuffer buffer;
	EXITypeClass valueType;

	buffer.buf = buf;
	buffer.bufContent = 0;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.ioStrm.readWriteToStream = NULL;
	buffer.ioStrm.stream = NULL;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header, if different from the defaults

	// III: Define an external stream for the output if any

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code = serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startDocument returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("http://www.ltu.se/EISLAB/schema-test", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("EXIPEncoder", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("version", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.attribute returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("0.2", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("status", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.attribute returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("alpha", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("This is an example of serializing EXI streams using EXIP low level API", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);

	tmp_err_code += serialize.endElement(&testStrm);
	tmp_err_code += serialize.endDocument(&testStrm);

	if(tmp_err_code != EXIP_OK)
		ck_assert_msg (tmp_err_code == EXIP_OK, "serialization ended with error code %d", tmp_err_code);

	// V: Free the memory allocated by the EXI stream object
	tmp_err_code = serialize.closeEXIStream(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.closeEXIStream ended with error code %d", tmp_err_code);


	buffer.bufContent = OUTPUT_BUFFER_SIZE;
	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object

	// IV: Parse the header of the stream

	tmp_err_code = parseHeader(&testParser, TRUE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);

	// V: Parse the body of the EXI stream

	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser object

	destroyParser(&testParser);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST

START_TEST (test_fragment_option)
{
	EXIStream testStrm;
	Parser testParser;
	String uri;
	String ln;
	QName qname= {&uri, &ln};
	String chVal;
	char buf[OUTPUT_BUFFER_SIZE];
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	BinaryBuffer buffer;
	EXITypeClass valueType;

	buffer.buf = buf;
	buffer.bufContent = 0;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.ioStrm.readWriteToStream = NULL;
	buffer.ioStrm.stream = NULL;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header, if different from the defaults
	testStrm.header.has_cookie = TRUE;
	testStrm.header.has_options = TRUE;
	SET_FRAGMENT(testStrm.header.opts.enumOpt);

	// III: Define an external stream for the output if any

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code = serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startDocument returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("http://www.ltu.se/EISLAB/schema-test", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("EXIPEncoder", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("version", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.attribute returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("0.2", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("status", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.attribute returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("alpha", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("Test", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("beta tests", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += asciiToStringManaged("Test2", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("beta tests -> second root element", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.endElement(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.endElement returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.endDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.endDocument returns an error code %d", tmp_err_code);

	if(tmp_err_code != EXIP_OK)
		ck_assert_msg (tmp_err_code == EXIP_OK, "serialization ended with error code %d", tmp_err_code);

	// V: Free the memory allocated by the EXI stream object
	tmp_err_code = serialize.closeEXIStream(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.closeEXIStream ended with error code %d", tmp_err_code);

	buffer.bufContent = OUTPUT_BUFFER_SIZE;

	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object

	// IV: Parse the header of the stream

	tmp_err_code = parseHeader(&testParser, FALSE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);
	// V: Parse the body of the EXI stream

	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser object

	destroyParser(&testParser);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST

START_TEST (test_value_part_zero)
{
	const String NS_EMPTY_STR = {NULL, 0};
	const String ELEM_COBS_STR = {"cobs", 4};
	const String ELEM_TM_STAMP_STR = {"timestamp", 9};
	const String ELEM_RPM_STR = {"rpm", 3};
	const String ELEM_ACC_RNDS_STR = {"accRounds", 9};
	const String ELEM_TEMP_STR = {"temp", 4};
	const String ELEM_LEFT_STR = {"left", 4};
	const String ELEM_RIGHT_STR = {"right", 5};
	const String ELEM_RSSI_STR = {"RSSI", 4};
	const String ELEM_MIN_STR = {"min", 3};
	const String ELEM_MAX_STR = {"max", 3};
	const String ELEM_AVG_STR = {"avg", 3};
	const String ELEM_BATTERY_STR = {"battery", 7};
	const String ATTR_NODE_ID_STR = {"nodeId", 6};

	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri;
	String ln;
	QName qname = {&uri, &ln, NULL};
	String chVal;
	BinaryBuffer buffer;
	EXITypeClass valueType;
	char valStr[50];
	char buf[OUTPUT_BUFFER_SIZE];
	Parser testParser;

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header, if different from the defaults
	testStrm.header.has_options = TRUE;
	testStrm.header.opts.valuePartitionCapacity = 0;

	// III: Define an external stream for the output if any
	buffer.ioStrm.readWriteToStream = NULL;
	buffer.ioStrm.stream = NULL;

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code += serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_COBS_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <cobs>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NODE_ID_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // nodeId="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%d", 111);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_TM_STAMP_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <timestamp>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	chVal.str = "2012-12-31T12:09:3.44";
	chVal.length = strlen("2012-12-31T12:09:3.44");
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </timestamp>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_RPM_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <rpm>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%d", 222);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </rpm>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_ACC_RNDS_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <accRounds>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%d", 4212);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </accRounds>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_TEMP_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <temp>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_LEFT_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <left>
	sprintf(valStr, "%f", 32.2);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </left>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_RIGHT_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <right>
	sprintf(valStr, "%f", 34.23);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </right>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </temp>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_RSSI_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <RSSI>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_AVG_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <avg>
	sprintf(valStr, "%d", 123);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </avg>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_MAX_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <max>
	sprintf(valStr, "%d", 2746);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </max>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_MIN_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <min>
	sprintf(valStr, "%d", 112);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </min>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </RSSI>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_BATTERY_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <battery>
	sprintf(valStr, "%f", 1.214);
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	tmp_err_code += serialize.endElement(&testStrm); // </battery>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </cobs>
	tmp_err_code += serialize.endDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	// VI: Free the memory allocated by the EXI stream object
	tmp_err_code += serialize.closeEXIStream(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	buffer.bufContent = OUTPUT_BUFFER_SIZE;
	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object

	// IV: Parse the header of the stream

	tmp_err_code = parseHeader(&testParser, TRUE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);
	// V: Parse the body of the EXI stream

	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser object

	destroyParser(&testParser);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST

START_TEST (test_recursive_defs)
{
	const String NS_EMPTY_STR = {NULL, 0};
	const String ELEM_OBJ_STR = {"obj", 3};
	const String ELEM_STR_STR = {"str", 3};
	const String ELEM_LIST_STR = {"list", 4};

	const String ATTR_XSTR_STR = {"xsss", 4};
	const String ATTR_XTEMP_STR = {"x-template", 10};
	const String ATTR_NAME_STR = {"name", 4};
	const String ATTR_VAL_STR = {"val", 3};

	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri;
	String ln;
	QName qname = {&uri, &ln, NULL};
	String chVal;
	BinaryBuffer buffer;
	EXITypeClass valueType;
	char valStr[50];
	char buf[OUTPUT_BUFFER_SIZE];
	Parser testParser;

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header, if different from the defaults
	testStrm.header.has_options = TRUE;
	SET_STRICT(testStrm.header.opts.enumOpt);
	SET_ALIGNMENT(testStrm.header.opts.enumOpt, BYTE_ALIGNMENT);

	// III: Define an external stream for the output if any
	buffer.ioStrm.readWriteToStream = NULL;
	buffer.ioStrm.stream = NULL;

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code += serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_OBJ_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <obj>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_XSTR_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // xsss="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "http://obix.org/ns/schema/1.1");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_XTEMP_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // x-template="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "ipu_inst.xml");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_STR_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NAME_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // name="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "interworkingProxyID");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_VAL_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // val="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "IPU_6LoWPAN");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_LIST_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <list>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NAME_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // name="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "supportedTechnologies");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ELEM_OBJ_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <obj>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	qname.localName = &ELEM_STR_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NAME_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // name="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "anPhysical");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_VAL_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // val="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "2003_2_4GHz");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	qname.localName = &ELEM_STR_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NAME_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // name="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "anStandard");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_VAL_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // val="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "Bee_1_0");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);


	qname.localName = &ELEM_STR_STR;
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType); // <str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_NAME_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // name="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "anProfile");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	qname.localName = &ATTR_VAL_STR;
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType); // val="..."
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	sprintf(valStr, "%s", "Bee_HA");
	chVal.str = valStr;
	chVal.length = strlen(valStr);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	tmp_err_code += serialize.endElement(&testStrm); // </str>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm); // </obj>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm); // </list>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm); // </obj>
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);
	// VI: Free the memory allocated by the EXI stream object
	tmp_err_code += serialize.closeEXIStream(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.* returns an error code %d", tmp_err_code);

	buffer.bufContent = OUTPUT_BUFFER_SIZE;
	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object

	// IV: Parse the header of the stream

	tmp_err_code = parseHeader(&testParser, FALSE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);

	// V: Parse the body of the EXI stream

	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser object

	destroyParser(&testParser);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST

errorCode encodeWithDynamicTypes(char* buf, int buf_size, int *strmSize);

/**
 * Example use of the xsi:type switch for dynamic typing of
 * element and attribute values. Note that schemaId must be set
 * to SCHEMA_ID_EMPTY in order to use the built-in schema types.
 * Encodes the following XML:
 *
 * <trivial xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 * xmlns:xsd="http://www.w3.org/2001/XMLSchema">
 *    <anElement xsi:type="xsd:date">2014-01-31T16:15:25+02:00</anElement>
 * </trivial>
 */
START_TEST (test_built_in_dynamic_types)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	char buf[OUTPUT_BUFFER_SIZE];
	int strmSize = 0;
	BinaryBuffer buffer;
	Parser testParser;

	tmp_err_code = encodeWithDynamicTypes(buf, OUTPUT_BUFFER_SIZE, &strmSize);
	ck_assert_msg(tmp_err_code == EXIP_OK, "There is an error in the encoding of dynamic types through xsi:type switch.");
	ck_assert_msg(strmSize > 0, "Encoding of dynamic types through xsi:type switch produces empty streams.");

	buffer.bufContent = strmSize;
	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object

	// IV: Parse the header of the stream

	tmp_err_code = parseHeader(&testParser, FALSE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);

	// V: Parse the body of the EXI stream

	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser object

	destroyParser(&testParser);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST

errorCode encodeWithDynamicTypes(char* buf, int buf_size, int *strmSize)
{
	const String NS_EMPTY = {NULL, 0};
	const String NS_XSI = {"http://www.w3.org/2001/XMLSchema-instance", 41};
	const String NS_XSD = {"http://www.w3.org/2001/XMLSchema", 32};

	const String ELEM_TRIVIAL = {"trivial", 7};
	const String ELEM_AN_ELEMENT = {"anElement", 9};

	const String PREFIX_XSI = {"xsi", 3};

	const String ATTR_TYPE = {"type", 4};

	const String VALUE_DATE = {"dateTime", 8};

	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri;
	String ln;
	QName qname = {&uri, &ln, NULL};

	BinaryBuffer buffer;
	EXITypeClass valueType;
	EXIPDateTime dt_val;

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header (including schemaID and schemaIDMode), if different from the defaults.
	testStrm.header.has_options = TRUE;
	SET_PRESERVED(testStrm.header.opts.preserve, PRESERVE_PREFIXES);
	testStrm.header.opts.schemaIDMode = SCHEMA_ID_EMPTY;

	// III: Define an external stream for the output if any, otherwise set to NULL
	buffer.ioStrm.readWriteToStream = NULL;
	buffer.ioStrm.stream = NULL;

	// IV: Initialize the stream
	TRY_CATCH_ENCODE(serialize.initStream(&testStrm, buffer, NULL));

	// V: Start building the stream step by step: header, document, element etc...
	TRY_CATCH_ENCODE(serialize.exiHeader(&testStrm));

	TRY_CATCH_ENCODE(serialize.startDocument(&testStrm));

	qname.uri = &NS_EMPTY;
	qname.localName = &ELEM_TRIVIAL;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <trivial>

	TRY_CATCH_ENCODE(serialize.namespaceDeclaration(&testStrm, NS_XSI, PREFIX_XSI, FALSE)); // xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"

	qname.uri = &NS_EMPTY;
	qname.localName = &ELEM_AN_ELEMENT;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <anElement>

	qname.uri = &NS_XSI;
	qname.localName = &ATTR_TYPE;
	TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, TRUE, &valueType)); // xsi:type="

	qname.uri = &NS_XSD;
	qname.localName = &VALUE_DATE;
	TRY_CATCH_ENCODE(serialize.qnameData(&testStrm, qname)); // xsd:date

	dt_val.dateTime.tm_year = 114; // + 1900 offset = 2014
	dt_val.dateTime.tm_mon = 0; // 0 = Jan
	dt_val.dateTime.tm_mday = 31;
	dt_val.dateTime.tm_hour = 16;
	dt_val.dateTime.tm_min = 15;
	dt_val.dateTime.tm_sec = 25;
	dt_val.presenceMask = 0;
	dt_val.presenceMask |= TZONE_PRESENCE;
	dt_val.TimeZone = 2*64; // UTC + 2
	TRY_CATCH_ENCODE(serialize.dateTimeData(&testStrm, dt_val)); // dateTime: 2014-01-31T16:15:25+02:00

	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </anElement>
	TRY_CATCH_ENCODE(serialize.endElement(&testStrm)); // </trivial>

	TRY_CATCH_ENCODE(serialize.endDocument(&testStrm));

	*strmSize = testStrm.context.bufferIndx + 1;

	// VI: Free the memory allocated by the EXI stream object
	TRY_CATCH_ENCODE(serialize.closeEXIStream(&testStrm));

	return EXIP_OK;
}


/* END: SchemaLess tests */

#define OUTPUT_BUFFER_SIZE_LARGE_DOC 20000
#define MAX_XSD_FILES_COUNT 10 // up to 10 XSD files

/* BEGIN: Schema-mode tests */
START_TEST (test_large_doc_str_pattern)
{
	const String NS_EMPTY_STR = {NULL, 0};

	const String ELEM_CONFIGURATION = {"configuration", 13};
	const String ELEM_CAPSWITCH = {"capable-switch", 14};
	const String ELEM_RESOURCES = {"resources", 9};
	const String ELEM_PORT = {"port", 4};
	const String ELEM_RESID = {"resource-id", 11};
	const String ELEM_ADMIN_STATE = {"admin-state", 11};
	const String ELEM_NORECEIVE = {"no-receive", 10};
	const String ELEM_NOFORWARD = {"no-forward", 10};
	const String ELEM_NOPACKET = {"no-packet-in", 12};

	const String ELEM_LOGSWITCHES = {"logical-switches", 16};
	const String ELEM_SWITCH = {"switch", 6};
	const String ELEM_ID = {"id", 2};
	const String ELEM_DATAPATHID = {"datapath-id", 11};
	const String ELEM_ENABLED = {"enabled", 7};
	const String ELEM_LOSTCONNBEH = {"lost-connection-behavior", 24};
	const String ELEM_CONTROLLERS = {"controllers", 11};
	const String ELEM_CONTROLLER = {"controller", 10};
	const String ELEM_ROLE = {"role", 4};
	const String ELEM_IPADDR = {"ip-address", 10};
	const String ELEM_PROTOCOL = {"protocol", 8};
	const String ELEM_STATE = {"state", 5};
	const String ELEM_CONNSTATE = {"connection-state", 16};
	const String ELEM_CURRVER = {"current-version", 15};

	const char * PORT_STR = "port";
	const char * SWITCH_STR = "switch";
	const char * STATE_UP_STR = "up";
	const char * DATAPATH_STR = "10:14:56:7C:89:46:7A:";
	const char * LOST_CONN_BEHAVIOR_STR = "failSecureMode";
	const char * CTRL_STR = "ctrl";
	const char * ROLE_STR = "equal";
	const char * IPADDR_STR = "10.10.10.";
	const char * PROTOCOL_STR = "tcp";
	const char * VER_STR = "1.0";

	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	FILE *outfile;
	char* sourceFile = "testOutputFile.exi";
	EXIPSchema schema;
//	struct timespec start;
//	struct timespec end;
	char* schemafname[1] = {"exip/schema_demo.exi"};
	EXIStream testStrm;
	String uri;
	String ln;
	QName qname = {&uri, &ln, NULL};
	String chVal;
	char buf[OUTPUT_BUFFER_SIZE_LARGE_DOC];
	BinaryBuffer buffer;
	int i, j;
	char strbuffer[32];

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE_LARGE_DOC;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	parseMultiSchema(schemafname, 1, &schema);

	outfile = fopen(sourceFile, "wb" );
	ck_assert_msg(outfile, "Unable to open file %s", sourceFile);

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header, if different from the defaults
	testStrm.header.has_cookie = TRUE;
	testStrm.header.has_options = TRUE;
	testStrm.header.opts.valueMaxLength = 300;
	testStrm.header.opts.valuePartitionCapacity = INDEX_MAX;
	SET_STRICT(testStrm.header.opts.enumOpt);

	// III: Define an external stream for the output if any
	buffer.ioStrm.readWriteToStream = writeFileOutputStream;
	buffer.ioStrm.stream = outfile;
	//buffer.ioStrm.readWriteToStream = NULL;
	//buffer.ioStrm.stream = NULL;
// printf("line:%d: %d\n", __LINE__, tmp_err_code);
	//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, &schema);
	ck_assert(tmp_err_code == EXIP_OK);

//printf("line:%d: %d\n", __LINE__, tmp_err_code);
//                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code += serialize.exiHeader(&testStrm);
// printf("line:%d: %d\n", __LINE__, tmp_err_code);
	tmp_err_code += serialize.startDocument(&testStrm);
// printf("line:%d: %d\n", __LINE__, tmp_err_code);
	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_CONFIGURATION;
	EXITypeClass typeClass;
	tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_CAPSWITCH;
	tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_RESOURCES;
	tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

// printf("line:%d: %d\n", __LINE__, tmp_err_code);
	for (i = 0; i < 100; i++)
	{
		qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_PORT;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_RESID;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	    sprintf(strbuffer, "%s%d", PORT_STR, i);
	    tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_CONFIGURATION;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_ADMIN_STATE;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += asciiToStringManaged(STATE_UP_STR, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_NORECEIVE;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += serialize.booleanData(&testStrm, FALSE);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_NOFORWARD;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += serialize.booleanData(&testStrm, FALSE);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_NOPACKET;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += serialize.booleanData(&testStrm, TRUE);
		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);
	}
	tmp_err_code += serialize.endElement(&testStrm);

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ELEM_LOGSWITCHES;
	tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	for (i = 0; i < 20; i++)
	{
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_SWITCH;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_ID;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

		sprintf(strbuffer, "%s%d", SWITCH_STR, i);
	    tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_DATAPATHID;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

		sprintf(strbuffer, "%s%d", DATAPATH_STR, 10 + i);
	    tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_ENABLED;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += serialize.booleanData(&testStrm, TRUE);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_LOSTCONNBEH;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += asciiToStringManaged(LOST_CONN_BEHAVIOR_STR, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
//		if ( i == 0 )
//		{
	        qname.uri = &NS_EMPTY_STR;
	        qname.localName = &ELEM_RESOURCES;
	        tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

			for (j = 0; j < 100; j++)
			{
	            qname.uri = &NS_EMPTY_STR;
	            qname.localName = &ELEM_PORT;
	            tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

				sprintf(strbuffer, "%s%d", PORT_STR, j);
	            tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	            tmp_err_code += serialize.stringData(&testStrm, chVal);
				tmp_err_code += serialize.endElement(&testStrm);
			}
			tmp_err_code += serialize.endElement(&testStrm);
//		}
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_CONTROLLERS;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_CONTROLLER;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_ID;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
		sprintf(strbuffer, "%s%d", CTRL_STR, i);
	    tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_ROLE;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += asciiToStringManaged(ROLE_STR, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_IPADDR;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
		sprintf(strbuffer, "%s%d", IPADDR_STR, i);
	    tmp_err_code += asciiToStringManaged(strbuffer, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_PORT;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += serialize.intData(&testStrm, 6620);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_PROTOCOL;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += asciiToStringManaged(PROTOCOL_STR, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_STATE;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);

	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_CONNSTATE;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
	    tmp_err_code += asciiToStringManaged(STATE_UP_STR, &chVal, &testStrm.memList, FALSE);
	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    qname.uri = &NS_EMPTY_STR;
	    qname.localName = &ELEM_CURRVER;
	    tmp_err_code += serialize.startElement(&testStrm, qname, &typeClass);
// printf("in loop(%d) line:%d: %d\n", i, __LINE__, tmp_err_code);
	    tmp_err_code += asciiToStringManaged(VER_STR, &chVal, &testStrm.memList, FALSE);

	    tmp_err_code += serialize.stringData(&testStrm, chVal);
		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);

		tmp_err_code += serialize.endElement(&testStrm);
	}

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += serialize.endElement(&testStrm);
	tmp_err_code += serialize.endDocument(&testStrm);
	ck_assert(tmp_err_code == EXIP_OK);

//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

	// VI: Free the memory allocated by the EXI stream object
	tmp_err_code = serialize.closeEXIStream(&testStrm);

//                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
//                total += ((end.tv_sec * SEC2NANO) + end.tv_nsec) - ((start.tv_sec * SEC2NANO) + start.tv_nsec);

    fclose(outfile);

    /* DECODE */
    {
    	Parser testParser;
    	FILE *infile;

    	buffer.buf = buf;
    	buffer.bufContent = 0;
    	buffer.bufLen = OUTPUT_BUFFER_SIZE_LARGE_DOC;
    	unsigned int eventCount;
		buffer.bufStrm = EMPTY_BUFFER_STREAM;

    	// Parsing steps:

    	// I.A: First, read in the schema
    	parseMultiSchema(schemafname, 1, &schema);

    	// I.B: Define an external stream for the input to the parser if any
    	infile = fopen(sourceFile, "rb" );
    	if(!infile)
    		ck_abort_msg("Unable to open file %s", sourceFile);

    	buffer.ioStrm.readWriteToStream = readFileInputStream;
    	buffer.ioStrm.stream = infile;

    	// II: Second, initialize the parser object
    	tmp_err_code = initParser(&testParser, buffer, &eventCount);
    	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

    	// IV: Parse the header of the stream
		tmp_err_code = parseHeader(&testParser, FALSE);
		ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

		tmp_err_code = setSchema(&testParser, &schema);
		ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);

		// V: Parse the body of the EXI stream
		while(tmp_err_code == EXIP_OK)
		{
			tmp_err_code = parseNext(&testParser);
		}

		// VI: Free the memory allocated by the parser object
		destroyParser(&testParser);
		fclose(infile);
		ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
    }

    remove(sourceFile);
	destroySchema(&schema);
}
END_TEST

#define INPUT_BUFFER_SIZE 200

/* Test substitution groups */
START_TEST (test_substitution_groups)
{
	EXIPSchema schema;
	FILE *infile;
	Parser testParser;
	char buf[INPUT_BUFFER_SIZE];
	char* schemafname[2] = {"exip/subsGroups/root-xsd.exi","exip/subsGroups/sub-xsd.exi"};
	char *exifname = "exip/subsGroups/root.exi";
	char exipath[MAX_PATH_LEN + strlen(exifname)];
	unsigned int eventCount;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	BinaryBuffer buffer;

	buffer.buf = buf;
	buffer.bufContent = 0;
	buffer.bufLen = INPUT_BUFFER_SIZE;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;
	
	// Parsing steps:

	// I.A: First, read in the schema
	parseMultiSchema(schemafname, 2, &schema);

	// I.B: Define an external stream for the input to the parser if any
	size_t pathlen = strlen(dataDir);
	memcpy(exipath, dataDir, pathlen);
	exipath[pathlen] = '/';
	memcpy(&exipath[pathlen+1], exifname, strlen(exifname)+1);

	infile = fopen(exipath, "rb" );
	if(!infile)
		ck_abort_msg("Unable to open file %s", exipath);

	buffer.ioStrm.readWriteToStream = readFileInputStream;
	buffer.ioStrm.stream = infile;

	// II: Second, initialize the parser object
	tmp_err_code = initParser(&testParser, buffer, &eventCount);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initParser returns an error code %d", tmp_err_code);

	// III: Initialize the parsing data and hook the callback handlers to the parser object
	eventCount = 0;

	// IV: Parse the header of the stream
	tmp_err_code = parseHeader(&testParser, FALSE);
	ck_assert_msg (tmp_err_code == EXIP_OK, "parsing the header returns an error code %d", tmp_err_code);

	tmp_err_code = setSchema(&testParser,  &schema);
	ck_assert_msg (tmp_err_code == EXIP_OK, "setSchema() returns an error code %d", tmp_err_code);
	// V: Parse the body of the EXI stream
	while(tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parseNext(&testParser);
		eventCount++;
	}

	ck_assert_msg(eventCount == 38,
	            "Unexpected event count: %u", eventCount);

	// VI: Free the memory allocated by the parser object
	destroyParser(&testParser);
	fclose(infile);
	ck_assert_msg (tmp_err_code == EXIP_PARSING_COMPLETE, "Error during parsing of the EXI body %d", tmp_err_code);
}
END_TEST


/* END: Schema-mode tests */

/* Helper functions */
static size_t writeFileOutputStream(void* buf, size_t readSize, void* stream)
{
	FILE *outfile = (FILE*) stream;
	return fwrite(buf, 1, readSize, outfile);
}

static size_t readFileInputStream(void* buf, size_t readSize, void* stream)
{
	FILE *infile = (FILE*) stream;
	return fread(buf, 1, readSize, infile);
}

static void parseMultiSchema(char** xsdList, int count, EXIPSchema* schema)
{
	FILE *schemaFile;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	size_t pathlen = strlen(dataDir);
	const size_t MAX_TOTAL_PATH_LEN = MAX_PATH_LEN*count + strlen(xsdList[0])*count;
	char exipath[MAX_TOTAL_PATH_LEN];
	char *pathPtr = exipath;

    memset(exipath, 0, MAX_TOTAL_PATH_LEN);

	for (int i = 0; i < count; i++)
	{
		memcpy(pathPtr, dataDir, pathlen);
		pathPtr[pathlen] = '/';
		memcpy(&pathPtr[pathlen+1], xsdList[i], strlen(xsdList[i])+1);
		schemaFile = fopen(pathPtr, "rb" );
		if(!schemaFile)
		{
			ck_abort_msg("Unable to open file %s", pathPtr);
			return;
		}
		else 
		{
			fclose(schemaFile);
			pathPtr += pathlen + 1 + strlen(xsdList[i]);
            if (i < (count - 1)){
                // Skip on the latest step
                pathPtr[0] = ',';
                pathPtr++;
            }
		}
	}

	tmp_err_code = parseSchema(exipath, NULL, schema);
	if (tmp_err_code == EXIP_MEMORY_ALLOCATION_ERROR) {
		ck_abort_msg("Memory allocation error!");
	}
	else if (tmp_err_code != EXIP_OK) 
	{
		ck_abort_msg("Error reading schema: %d", tmp_err_code);
	}
}

Suite* exip_suite(void)
{
	Suite *s = suite_create("EXIP");
	{
		/* Schema-less test case */
		TCase *tc_SchLess = tcase_create ("SchemaLess");
		tcase_add_test (tc_SchLess, test_default_options);
		tcase_add_test (tc_SchLess, test_fragment_option);
		tcase_add_test (tc_SchLess, test_value_part_zero);
		tcase_add_test (tc_SchLess, test_recursive_defs);
		tcase_add_test (tc_SchLess, test_built_in_dynamic_types);
		suite_add_tcase (s, tc_SchLess);
	}
	{
		/* Schema-mode test case */
		TCase *tc_Schema = tcase_create ("Schema-mode");
		tcase_add_test (tc_Schema, test_large_doc_str_pattern);
		tcase_add_test (tc_Schema, test_substitution_groups);
		suite_add_tcase (s, tc_Schema);
	}

	return s;
}

int main (int argc, char *argv[])
{
	int number_failed;
	Suite *s = exip_suite();
	SRunner *sr = srunner_create (s);

	if (argc < 2)
	{
		printf("ERR: Expected test data directory\n");
		exit(1);
	}
	if (strlen(argv[1]) > MAX_PATH_LEN)
	{
		printf("ERR: Test data pathname too long: %u", (unsigned int) strlen(argv[1]));
		exit(1);
	}

	dataDir = argv[1];

#ifdef _MSC_VER
	srunner_set_fork_status(sr, CK_NOFORK);
#endif
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

