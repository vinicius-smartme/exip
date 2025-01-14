/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file check_profile.c
 * @brief Tests the EXI Profile processing
 *
 * @date Aug 6, 2013
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

#define OUTPUT_BUFFER_SIZE 2000

/* BEGIN: Interoperability tests */

START_TEST (test_noLearning01)
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
	testStrm.header.has_options = TRUE;
	testStrm.header.opts.schemaIDMode = SCHEMA_ID_EMPTY;

	// III: Define an external stream for the output if any

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code = serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startDocument returns an error code %d", tmp_err_code);

	getEmptyString(&uri);
	tmp_err_code += asciiToStringManaged("root", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("a", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("bla", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += asciiToStringManaged("b", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("23", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

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

START_TEST (test_noLearning02)
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
	testStrm.header.has_options = TRUE;
	testStrm.header.opts.schemaIDMode = SCHEMA_ID_EMPTY;

	// III: Define an external stream for the output if any

	// IV: Initialize the stream
	tmp_err_code = serialize.initStream(&testStrm, buffer, NULL);
	ck_assert_msg (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

	// V: Start building the stream step by step: header, document, element etc...
	tmp_err_code = serialize.exiHeader(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.exiHeader returns an error code %d", tmp_err_code);

	tmp_err_code = serialize.startDocument(&testStrm);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startDocument returns an error code %d", tmp_err_code);

	getEmptyString(&uri);
	tmp_err_code += asciiToStringManaged("root", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("a", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("bla", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += asciiToStringManaged("b", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("23", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += asciiToStringManaged("a", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("bla", &chVal, &testStrm.memList, FALSE);
	tmp_err_code += serialize.stringData(&testStrm, chVal);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += asciiToStringManaged("c", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("b", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.startElement(&testStrm, qname, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.startElement returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("http://www.w3.org/2001/XMLSchema-instance", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("type", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.attribute(&testStrm, qname, TRUE, &valueType);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.attribute returns an error code %d", tmp_err_code);

	tmp_err_code += asciiToStringManaged("http://www.w3.org/2001/XMLSchema", &uri, &testStrm.memList, FALSE);
	tmp_err_code += asciiToStringManaged("integer", &ln, &testStrm.memList, FALSE);
	tmp_err_code += serialize.qnameData(&testStrm, qname);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.qnameData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.intData(&testStrm, 66);
	ck_assert_msg (tmp_err_code == EXIP_OK, "serialize.stringData returns an error code %d", tmp_err_code);

	tmp_err_code += serialize.endElement(&testStrm);

	tmp_err_code += serialize.endElement(&testStrm);

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


/* END: Interoperability tests */

Suite * profile_suite (void)
{
  Suite *s = suite_create ("EXI Profile");

  {
	  /* Grammars test case */
	  TCase *tc_intProfile = tcase_create ("Interoperability tests");
	  tcase_add_test (tc_intProfile, test_noLearning01);
	  tcase_add_test (tc_intProfile, test_noLearning02);
//	  tcase_add_test (tc_intProfile, test_noLearning03);
//	  tcase_add_test (tc_intProfile, test_noLearning04);
//	  tcase_add_test (tc_intProfile, test_noLearning05);
	  suite_add_tcase (s, tc_intProfile);
  }

  return s;
}

int main (void)
{
	int number_failed;
	Suite *s = profile_suite();
	SRunner *sr = srunner_create (s);

#ifdef _MSC_VER
	srunner_set_fork_status(sr, CK_NOFORK);
#endif
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
