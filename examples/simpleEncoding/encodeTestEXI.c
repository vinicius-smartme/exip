/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file encodeTestEXI.c
 * @brief Testing the EXI encoder
 *
 * @date Nov 4, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "encodeTestEXI.h"
#include "EXISerializer.h"
#include "stringManipulate.h"
#include <stdio.h>
#include <string.h>

#define OUTPUT_BUFFER_SIZE 200

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
//static String ENUM_DATA_1 = {"hello", 5};
//static String ENUM_DATA_2 = {"hi", 2};
//static String ENUM_DATA_3 = {"hey", 3};
static String ENUM_DATA_4 = {"hej", 3};

#define TRY_CATCH_ENCODE(func) TRY_CATCH(func, serialize.closeEXIStream(&testStrm))

errorCode encode(EXIPSchema* schemaPtr, FILE *outfile, size_t (*outputStream)(void* buf, size_t size, void* stream))
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIStream testStrm;
	String uri;
	String ln;
	QName qname = {&uri, &ln, NULL};
	String chVal;
	char buf[OUTPUT_BUFFER_SIZE];
	BinaryBuffer buffer;
	EXITypeClass valueType;

	buffer.buf = buf;
	buffer.bufLen = OUTPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;

	// Serialization steps:

	// I: First initialize the header of the stream
	serialize.initHeader(&testStrm);

	// II: Set any options in the header (including schemaID and schemaIDMode), if different from the defaults.
	testStrm.header.has_cookie = TRUE;
	testStrm.header.has_options = TRUE;
	testStrm.header.opts.valueMaxLength = 300;
	testStrm.header.opts.valuePartitionCapacity = 50;
	SET_STRICT(testStrm.header.opts.enumOpt);

	// III: Define an external stream for the output if any, otherwise set to NULL
	buffer.ioStrm.readWriteToStream = outputStream;
	buffer.ioStrm.stream = outfile;

	// IV: Initialize the stream
	TRY_CATCH_ENCODE(serialize.initStream(&testStrm, buffer, schemaPtr));

	// V: Start building the stream step by step: header, document, element etc...
	TRY_CATCH_ENCODE(serialize.exiHeader(&testStrm));

	TRY_CATCH_ENCODE(serialize.startDocument(&testStrm));

	qname.uri = &NS_STR;
	qname.localName = &ELEM_MULT_TEST_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <MultipleXSDsTest>

	qname.uri = &NS_STR;
	qname.localName = &ELEM_ENCODE_STR;
	TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType)); // <EXIPEncoder>

	// NOTE: attributes should come lexicographically sorted during serialization

	qname.uri = &NS_EMPTY_STR;
	qname.localName = &ATTR_BYTE_STR;

	if(schemaPtr != NULL)
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

	if(schemaPtr != NULL)
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

	if(schemaPtr != NULL)
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

	if(schemaPtr != NULL)
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

	if(schemaPtr != NULL)
	{
		// schema mode
		EXIPDateTime dt;

		dt.presenceMask = FRACT_PRESENCE;

		dt.dateTime.tm_year = 112; // 2012
		dt.dateTime.tm_mon = 6;	// July
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

	if(schemaPtr != NULL)
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

	return tmp_err_code;
}
