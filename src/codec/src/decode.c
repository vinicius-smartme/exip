/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file decode.c
 * @brief An EXI decoder
 */

#include "decode.h"
#include "codec_common.h"
#include "EXIParser.h"
#include "stringManipulate.h"
#include "parseSchema.h"
#include "../../grammarGen/include/grammarGenerator.h"
#include <stdio.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 200
#define MAX_PREFIXES 10

struct appData
{
	unsigned char outputFormat;
	unsigned char expectAttributeData;
	char nameBuf[200];				  // needed for the OUT_XML Output Format
	struct element *stack;			  // needed for the OUT_XML Output Format
	unsigned char unclosedElement;	  // needed for the OUT_XML Output Format
	char prefixes[MAX_PREFIXES][200]; // needed for the OUT_XML Output Format
	unsigned char prefixesCount;	  // needed for the OUT_XML Output Format
	/**
	 * @brief Decoded data to be outputted
	 *
	 */
	List outData;
};

// Stuff needed for the OUT_XML Output Format
// ******************************************
struct element
{
	struct element *next;
	char *name;
};

static void push(struct element **stack, struct element *el);
static struct element *pop(struct element **stack);
static struct element *createElement(char *name);
static void destroyElement(struct element *el);

// returns != 0 if error
static char lookupPrefix(struct appData *aData, String ns, unsigned char *prxHit, unsigned char *prefixIndex);

// ******************************************

// Content Handler API
static errorCode sample_fatalError(const errorCode code, const char *msg, void *app_data);
static errorCode sample_startDocument(void *app_data);
static errorCode sample_endDocument(void *app_data);
static errorCode sample_startElement(QName qname, void *app_data);
static errorCode sample_endElement(void *app_data);
static errorCode sample_attribute(QName qname, void *app_data);
static errorCode sample_stringData(const String value, void *app_data);
static errorCode sample_decimalData(Decimal value, void *app_data);
static errorCode sample_intData(Integer int_val, void *app_data);
static errorCode sample_floatData(Float fl_val, void *app_data);
static errorCode sample_booleanData(boolean bool_val, void *app_data);
static errorCode sample_dateTimeData(EXIPDateTime dt_val, void *app_data);
static errorCode sample_binaryData(const char *binary_val, Index nbytes, void *app_data);
static errorCode sample_qnameData(const QName qname, void *app_data);

static errorCode decode(
	EXIPSchema *schemaPtr,
	unsigned char outFlag,
	boolean outOfBandOpts,
	EXIOptions *opts,
	void *inputFilePath,
	size_t (*inputStream)(void *buf, size_t size, void *stream),
	void *inData,
	size_t inDataLen,
	List *outData)
{
	Parser testParser;
	char buf[INPUT_BUFFER_SIZE];
	BinaryBuffer buffer;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	struct appData parsingData;

	buffer.buf = buf;
	buffer.bufLen = INPUT_BUFFER_SIZE;
	buffer.bufContent = 0;
	buffer.bufStrm = EMPTY_BUFFER_STREAM;
	// Parsing steps:

	// I: First, define an external stream for the input to the parser if any, otherwise tries as external buffer
	buffer.ioStrm.readWriteToStream = inputStream;
	buffer.ioStrm.stream = inputFilePath;
	if (inputStream == NULL && inData != NULL && inDataLen > 0)
	{
		buffer.bufStrm.buf = inData;
		buffer.bufStrm.bufContent = inDataLen;
		buffer.bufStrm.bufLen = inDataLen;
	}

	// II: Second, initialize the parser object
	TRY(parse.initParser(&testParser, buffer, &parsingData));

	// III: Initialize the parsing data and hook the callback handlers to the parser object.
	//      If out-of-band options are defined use testParser.strm.header.opts to set them
	parsingData.expectAttributeData = 0;
	parsingData.stack = NULL;
	parsingData.unclosedElement = 0;
	parsingData.prefixesCount = 0;
	parsingData.outputFormat = outFlag;
	parsingData.outData = newList();
	if (outOfBandOpts && opts != NULL)
		testParser.strm.header.opts = *opts;

	testParser.handler.fatalError = sample_fatalError;
	testParser.handler.error = sample_fatalError;
	testParser.handler.startDocument = sample_startDocument;
	testParser.handler.endDocument = sample_endDocument;
	testParser.handler.startElement = sample_startElement;
	testParser.handler.attribute = sample_attribute;
	testParser.handler.stringData = sample_stringData;
	testParser.handler.endElement = sample_endElement;
	testParser.handler.decimalData = sample_decimalData;
	testParser.handler.intData = sample_intData;
	testParser.handler.floatData = sample_floatData;
	testParser.handler.booleanData = sample_booleanData;
	testParser.handler.dateTimeData = sample_dateTimeData;
	testParser.handler.binaryData = sample_binaryData;
	testParser.handler.qnameData = sample_qnameData;

	// IV: Parse the header of the stream

	TRY(parse.parseHeader(&testParser, outOfBandOpts));

	// IV.1: Set the schema to be used for parsing.
	// The schemaID mode and schemaID field can be read at
	// parser.strm.header.opts.schemaIDMode and
	// parser.strm.header.opts.schemaID respectively
	// If schemaless mode, use setSchema(&parser, NULL);

	TRY(parse.setSchema(&testParser, schemaPtr));

	// V: Parse the body of the EXI stream

	while (tmp_err_code == EXIP_OK)
	{
		tmp_err_code = parse.parseNext(&testParser);
	}

	// VI: Free the memory allocated by the parser

	parse.destroyParser(&testParser);

	outData->size = parsingData.outData.size;
	outData->head = parsingData.outData.head;
	outData->tail = parsingData.outData.tail;

	printf("Output size: %d (%p)\n", outData->size, (void *)outData);

	if (tmp_err_code == EXIP_PARSING_COMPLETE)
		return EXIP_OK;
	else
		return tmp_err_code;
}

errorCode decodeFromFile(
	char *schemaPath,
	unsigned char outFlag,
	boolean hasOptions,
	EXIOptions *options,
	const char *inputFilePath,
	List *outData)
{
	EXIPSchema schema;
	EXIPSchema* schemaPtr = NULL;
	void *inputFile;
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

	inputFile = fopen(inputFilePath, "rb");
	if (!inputFile)
	{
		fprintf(stderr, "Unable to open XML file \"%s\" for parsing\n", inputFilePath);
		return EXIP_INVALID_INPUT;
	}

	ret = decode(
		schemaPtr,
		outFlag,
		hasOptions,
		options,
		inputFile,
		readFileInputStream,
		NULL,
		0,
		outData);

	if(schemaPtr != NULL)
		destroySchema(schemaPtr);
	fclose(inputFile);
	return ret;
}

errorCode decodeFromBuffer(
	char *schemaPath,
	unsigned char outFlag,
	boolean hasOptions,
	EXIOptions *options,
	void *inData,
	size_t inDataLen,
	List *outData)
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

	ret = decode(
		schemaPtr,
		outFlag,
		hasOptions,
		options,
		NULL,
		NULL,
		inData,
		inDataLen,
		outData);

	if(schemaPtr != NULL)
		destroySchema(schemaPtr);

	return ret;
}

/**
 * @brief Update the last attribute located on the List tail if isAttribute is true.
 * Otherwise, add a new entry to the list.
 *
 * @param isAttribute If the msg item is an attribute. Otherwise, considered as a new entry.
 * @param list The list to be updated.
 * @param msg The message to be added to the list.
 */
static errorCode updateListLastAttribute(unsigned char isAttribute, List *list, char *msg)
{
	size_t elementSize = 0;
	if (isAttribute)
	{
		// This is not a new list entry. So we need to get the tail from the list
		// and append the new string to it.
		Node *tail;

		if (list->tail != NULL)
		{
			tail = list->tail;
		}
		else
		{
			printf("Error: Failed to get the Tail from the list.\n");
			elementSize = list->size;
			tail = getNth(list, elementSize);
		}

		elementSize = tail->size;
		size_t msgLen = strlen(msg) + 1;
		char *msg_buffer = calloc(elementSize + msgLen, sizeof(char));
		if (!msg_buffer)
		{
			fprintf(stderr, "Memory allocation error!");
			return EXIP_MEMORY_ALLOCATION_ERROR;
		}
		strcpy(msg_buffer, tail->data);
		free(tail->data);
		strcpy(msg_buffer + elementSize, msg);
		tail->data = msg_buffer;
		tail->size = strlen(msg_buffer);
	}
	else
	{
		pushBack(list, (void *)msg, strlen(msg));
	}

	return EXIP_OK;
}
static errorCode sample_fatalError(const errorCode code, const char *msg, void *app_data)
{
	char err_msg[128];
	sprintf(err_msg, "\n%d : FATAL ERROR: %s\n", code, msg);
	printf("%s", err_msg);
	struct appData *appD = (struct appData *)app_data;
	pushBack(&appD->outData, err_msg, strlen(msg));
	return EXIP_HANDLER_STOP;
}

static errorCode sample_startDocument(void *app_data)
{
	char msg[40];
	struct appData *appD = (struct appData *)app_data;
	if (appD->outputFormat == OUT_EXI)
		sprintf(msg, "SD\n");
	else if (appD->outputFormat == OUT_XML)
		sprintf(msg, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

	printf("%s", msg);
	pushBack(&appD->outData, msg, strlen(msg));
	return EXIP_OK;
}

static errorCode sample_endDocument(void *app_data)
{
	char msg[4];
	struct appData *appD = (struct appData *)app_data;
	if (appD->outputFormat == OUT_EXI)
		sprintf(msg, "ED\n");
	else if (appD->outputFormat == OUT_XML)
		sprintf(msg, "\n");

	printf("%s", msg);
	pushBack(&appD->outData, msg, strlen(msg));
	return EXIP_OK;
}

static errorCode sample_startElement(QName qname, void *app_data)
{
	char msg[300];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		printf("SE ");
		sprintf(msg, "SE ");
		msgIdx += 3;
		printString(qname.uri);
		sPrintString(msg + msgIdx, qname.uri);
		msgIdx = strlen(msg);
		printf(" ");
		sprintf(msg + msgIdx, " ");
		msgIdx++;
		printString(qname.localName);
		sPrintString(msg + msgIdx, qname.localName);
		msgIdx = strlen(msg);
		printf("\n");
		sprintf(msg + msgIdx, "\n");
	}
	else if (appD->outputFormat == OUT_XML)
	{
		char error = 0;
		unsigned char prefixIndex = 0;
		unsigned char prxHit = 1;
		int t;

		if (!isStringEmpty(qname.uri))
		{
			error = lookupPrefix(appD, *qname.uri, &prxHit, &prefixIndex);
			if (error != 0)
				return EXIP_HANDLER_STOP;

			sprintf(appD->nameBuf, "p%d:", prefixIndex);
			t = strlen(appD->nameBuf);
			memcpy(appD->nameBuf + t, qname.localName->str, qname.localName->length);
			appD->nameBuf[t + qname.localName->length] = '\0';
		}
		else
		{
			memcpy(appD->nameBuf, qname.localName->str, qname.localName->length);
			appD->nameBuf[qname.localName->length] = '\0';
		}
		push(&(appD->stack), createElement(appD->nameBuf));
		if (appD->unclosedElement)
		{
			printf(">\n");
			// sprintf(msg, ">\n");
			// msgIdx++;
			tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">\n");
			if (tmp_err_code != EXIP_OK) {
				fprintf(stderr, "Unable to update list at sample_startElement - error code: %d", tmp_err_code);
				return tmp_err_code;
			}
		}
		printf("<%s", appD->nameBuf);
		sprintf(msg + msgIdx, "<%s", appD->nameBuf);
		msgIdx += strlen(appD->nameBuf) + 1;

		if (prxHit == 0)
		{
			sprintf(appD->nameBuf, " xmlns:p%d=\"", prefixIndex);
			printf("%s", appD->nameBuf);
			sprintf(msg + msgIdx, "%s", appD->nameBuf);
			msgIdx += strlen(appD->nameBuf);

			printString(qname.uri);
			printf("\"");
			sprintf(msg + msgIdx, "%.*s\"", (int)qname.uri->length, qname.uri->str);
		}

		appD->unclosedElement = 1;
	}

	pushBack(&appD->outData, msg, strlen(msg));
	return EXIP_OK;
}

static errorCode sample_endElement(void *app_data)
{
	char msg[128];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		sprintf(msg, "EE\n");
		printf("%s", msg);
	}
	else if (appD->outputFormat == OUT_XML)
	{
		struct element *el;

		if (appD->unclosedElement)
		{
			printf(">\n");
			// sprintf(msg, ">\n");
			// msgIdx++;
			tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">\n");
			if (tmp_err_code != EXIP_OK) {
				fprintf(stderr, "Unable to update list at sample_endElement - error code: %d", tmp_err_code);
				return tmp_err_code;
			}
		}
		appD->unclosedElement = 0;
		el = pop(&(appD->stack));
		printf("</%s>\n", el->name);
		sprintf(msg + msgIdx, "</%s>\n", el->name);
		destroyElement(el);
	}

	pushBack(&appD->outData, msg, strlen(msg));
	return EXIP_OK;
}

static errorCode sample_attribute(QName qname, void *app_data)
{
	char msg[128];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	if (appD->outputFormat == OUT_EXI)
	{
		printf("AT ");
		sprintf(msg + msgIdx, "AT ");
		msgIdx += 3;
		printString(qname.uri);
		sPrintString(msg + msgIdx, qname.uri);
		msgIdx = strlen(msg);
		printf(" ");
		sprintf(msg + msgIdx, " ");
		msgIdx++;
		printString(qname.localName);
		sPrintString(msg + msgIdx, qname.localName);
		msgIdx = strlen(msg);
		printf("=\"");
		sprintf(msg + msgIdx, "=\"");
	}
	else if (appD->outputFormat == OUT_XML)
	{
		printf(" ");
		sprintf(msg + msgIdx, " ");
		msgIdx++;
		if (!isStringEmpty(qname.uri))
		{
			printString(qname.uri);
			sPrintString(msg + msgIdx, qname.uri);
			msgIdx = strlen(msg);
			printf(":");
			sprintf(msg + msgIdx, ":");
			msgIdx++;
		}
		printString(qname.localName);
		sPrintString(msg + msgIdx, qname.localName);
		msgIdx = strlen(msg);
		printf("=\"");
		sprintf(msg + msgIdx, "=\"");
	}
	appD->expectAttributeData = 1;

	pushBack(&appD->outData, msg, strlen(msg));
	return EXIP_OK;
}

static errorCode sample_stringData(const String value, void *app_data)
{
	char msg[300];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			printString(&value);
			sPrintString(msg + msgIdx, &value);
			msgIdx = strlen(msg);
			printf("\"\n");
			sprintf(msg + msgIdx, "\"\n");
			msgIdx += 2;
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			sprintf(msg + msgIdx, "CH ");
			msgIdx += 3;
			printString(&value);
			sPrintString(msg + msgIdx, &value);
			msgIdx = strlen(msg);
			printf("\n");
			sprintf(msg + msgIdx, "\n");
			msgIdx++;
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			printString(&value);
			sPrintString(msg + msgIdx, &value);
			msgIdx = strlen(msg);
			printf("\"");
			sprintf(msg + msgIdx, "\"");
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_stringData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			printString(&value);
			sPrintString(msg + msgIdx, &value);
			msgIdx = strlen(msg);
			// sprintf(msg + msgIdx, "\0");
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list msg at sample_stringData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}

	return EXIP_OK;
}

static errorCode sample_decimalData(Decimal value, void *app_data)
{
	return sample_floatData(value, app_data);
}

static errorCode sample_intData(Integer int_val, void *app_data)
{
	char msg[35];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	char tmp_buf[30];
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			sprintf(tmp_buf, "%lld", (long long int)int_val);
			printf("%s", tmp_buf);
			printf("\"\n");
			sprintf(msg + msgIdx, "%s\"\n", tmp_buf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			sprintf(tmp_buf, "%lld", (long long int)int_val);
			printf("%s", tmp_buf);
			printf("\n");
			sprintf(msg + msgIdx, "CH %s\"\n", tmp_buf);
			msgIdx = strlen(msg);
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			sprintf(tmp_buf, "%lld", (long long int)int_val);
			printf("%s", tmp_buf);
			printf("\"");
			sprintf(msg + msgIdx, "%s\"", tmp_buf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_intData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			sprintf(tmp_buf, "%lld", (long long int)int_val);
			printf("%s", tmp_buf);
			sprintf(msg + msgIdx, "%s", tmp_buf);
			msgIdx = strlen(msg);
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list at sample_intData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

static errorCode sample_booleanData(boolean bool_val, void *app_data)
{
	char msg[11];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			if (bool_val)
			{
				printf("true\"\n");
				sprintf(msg + msgIdx, "true\"\n");
				msgIdx += 7;
			}
			else
			{
				printf("false\"\n");
				sprintf(msg + msgIdx, "false\"\n");
				msgIdx += 8;
			}

			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			if (bool_val)
			{
				printf("true\n");
				sprintf(msg + msgIdx, "CH true\"\n");
				msgIdx += 10;
			}
			else
				printf("false\n");
			sprintf(msg + msgIdx, "CH false\"\n");
			msgIdx += 11;
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			if (bool_val)
			{
				printf("true\"");
				sprintf(msg + msgIdx, "true\"");
				msgIdx += 6;
			}
			else
			{
				printf("false\"");
				sprintf(msg + msgIdx, "false\"");
				msgIdx += 7;
			}
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_booleanData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;

			if (bool_val)
			{
				printf("true");
				sprintf(msg + msgIdx, "true");
				msgIdx += 5;
			}
			else
			{
				printf("false");
				sprintf(msg + msgIdx, "false");
				msgIdx += 6;
			}
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list at sample_booleanData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

static errorCode sample_floatData(Float fl_val, void *app_data)
{
	char msg[35];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	char tmp_buf[30];
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			sprintf(tmp_buf, "%lldE%d", (long long int)fl_val.mantissa, fl_val.exponent);
			printf("%s", tmp_buf);
			printf("\"\n");
			sprintf(msg + msgIdx, "%s\"\n", tmp_buf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			sprintf(tmp_buf, "%lldE%d", (long long int)fl_val.mantissa, fl_val.exponent);
			printf("%s", tmp_buf);
			printf("\n");
			sprintf(msg + msgIdx, "CH %s\n", tmp_buf);
			msgIdx = strlen(msg);
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			sprintf(tmp_buf, "%lldE%d", (long long int)fl_val.mantissa, fl_val.exponent);
			printf("%s", tmp_buf);
			printf("\"");
			sprintf(msg + msgIdx, "%s\"", tmp_buf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_floatData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			sprintf(tmp_buf, "%lldE%d", (long long int)fl_val.mantissa, fl_val.exponent);
			printf("%s", tmp_buf);
			sprintf(msg + msgIdx, "%s", tmp_buf);
			msgIdx = strlen(msg);
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list msg at sample_floatData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

static errorCode sample_dateTimeData(EXIPDateTime dt_val, void *app_data)
{
	char msg[90];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	char fsecBuf[30];
	char tzBuf[30];
	int i;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (IS_PRESENT(dt_val.presenceMask, FRACT_PRESENCE))
	{
		unsigned int tmpfValue = dt_val.fSecs.value;
		int digitNum = 0;

		fsecBuf[0] = '.';

		while (tmpfValue)
		{
			digitNum++;
			tmpfValue = tmpfValue / 10;
		}
		for (i = 0; i < dt_val.fSecs.offset + 1 - digitNum; i++)
			fsecBuf[1 + i] = '0';

		sprintf(fsecBuf + 1 + i, "%d", dt_val.fSecs.value);
	}
	else
	{
		fsecBuf[0] = '\0';
	}

	if (IS_PRESENT(dt_val.presenceMask, TZONE_PRESENCE))
	{
		if (dt_val.TimeZone < 0)
			tzBuf[0] = '-';
		else
			tzBuf[0] = '+';
		sprintf(tzBuf + 1, "%02d", dt_val.TimeZone / 64);
		tzBuf[3] = ':';
		sprintf(tzBuf + 4, "%02d", dt_val.TimeZone % 64);
		tzBuf[6] = '\0';
	}
	else
	{
		tzBuf[0] = '\0';
	}

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s", dt_val.dateTime.tm_year + 1900,
				   dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
				   dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
				   dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			printf("\"\n");
			sprintf(msg + msgIdx, "%04d-%02d-%02dT%02d:%02d:%02d%s%s\"\n",
					dt_val.dateTime.tm_year + 1900,
					dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
					dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
					dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s", dt_val.dateTime.tm_year + 1900,
				   dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
				   dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
				   dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			printf("\n");
			sprintf(msg + msgIdx, "CH %04d-%02d-%02dT%02d:%02d:%02d%s%s\n",
					dt_val.dateTime.tm_year + 1900,
					dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
					dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
					dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			msgIdx = strlen(msg);
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s", dt_val.dateTime.tm_year + 1900,
				   dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
				   dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
				   dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			printf("\"");
			sprintf(msg + msgIdx, "%04d-%02d-%02dT%02d:%02d:%02d%s%s\"",
					dt_val.dateTime.tm_year + 1900,
					dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
					dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
					dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_dateTimeData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s", dt_val.dateTime.tm_year + 1900,
				   dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
				   dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
				   dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			sprintf(msg + msgIdx, "%04d-%02d-%02dT%02d:%02d:%02d%s%s",
					dt_val.dateTime.tm_year + 1900,
					dt_val.dateTime.tm_mon + 1, dt_val.dateTime.tm_mday,
					dt_val.dateTime.tm_hour, dt_val.dateTime.tm_min,
					dt_val.dateTime.tm_sec, fsecBuf, tzBuf);
			msgIdx = strlen(msg);
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list msg at sample_dateTimeData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

static errorCode sample_binaryData(const char *binary_val, Index nbytes, void *app_data)
{
	char msg[30];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			printf("[binary: %d bytes]", (int)nbytes);
			printf("\"\n");
			sprintf(msg + msgIdx, "[binary: %d bytes]\"\n", (int)nbytes);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("CH ");
			printf("[binary: %d bytes]", (int)nbytes);
			printf("\n");
			sprintf(msg + msgIdx, "CH [binary: %d bytes]\n", (int)nbytes);
			msgIdx = strlen(msg);
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			printf("[binary: %d bytes]", (int)nbytes);
			printf("\"");
			sprintf(msg + msgIdx, "[binary: %d bytes]\"", (int)nbytes);
			msgIdx = strlen(msg);
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_binaryData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			printf("[binary: %d bytes]", (int)nbytes);
			sprintf(msg + msgIdx, "[binary: %d bytes]", (int)nbytes);
			msgIdx = strlen(msg);
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list msg at sample_binaryData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

static errorCode sample_qnameData(const QName qname, void *app_data)
{
	char msg[128];
	size_t msgIdx = 0;
	struct appData *appD = (struct appData *)app_data;
	unsigned char isAttribute = appD->expectAttributeData;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	if (appD->outputFormat == OUT_EXI)
	{
		if (appD->expectAttributeData)
		{
			printString(qname.uri);
			sPrintString(msg + msgIdx, qname.uri);
			msgIdx = strlen(msg);
			printf(":");
			sprintf(msg + msgIdx, ":");
			msgIdx++;
			printString(qname.localName);
			sPrintString(msg + msgIdx, qname.localName);
			msgIdx = strlen(msg);
			printf("\"\n");
			sprintf(msg + msgIdx, "\"\n");
			appD->expectAttributeData = 0;
		}
		else
		{
			printf("QNAME ");
			sprintf(msg + msgIdx, "QNAME ");
			msgIdx += 6;
			printString(qname.uri);
			sPrintString(msg + msgIdx, qname.uri);
			msgIdx = strlen(msg);
			printf(":");
			sprintf(msg + msgIdx, ":");
			msgIdx++;
			printString(qname.localName);
			sPrintString(msg + msgIdx, qname.localName);
			msgIdx = strlen(msg);
			printf("\n");
			sprintf(msg + msgIdx, "\n");
		}
	}
	else if (appD->outputFormat == OUT_XML)
	{
		if (appD->expectAttributeData)
		{
			printString(qname.uri);
			sPrintString(msg + msgIdx, qname.uri);
			msgIdx = strlen(msg);
			printf(":");
			sprintf(msg + msgIdx, ":");
			msgIdx++;
			printString(qname.localName);
			sPrintString(msg + msgIdx, qname.localName);
			msgIdx = strlen(msg);
			printf("\"");
			sprintf(msg + msgIdx, "\"");
			appD->expectAttributeData = 0;
		}
		else
		{
			if (appD->unclosedElement)
			{
				printf(">");
				// sprintf(msg + msgIdx, ">");
				// msgIdx++;
				tmp_err_code = updateListLastAttribute(1, &(appD->outData), ">");
				if (tmp_err_code != EXIP_OK) {
					fprintf(stderr, "Unable to update list at sample_qnameData - error code: %d", tmp_err_code);
					return tmp_err_code;
				}
			}
			appD->unclosedElement = 0;
			printString(qname.uri);
			sPrintString(msg + msgIdx, qname.uri);
			msgIdx = strlen(msg);
			printf(":");
			sprintf(msg + msgIdx, ":");
			msgIdx++;
			printString(qname.localName);
			sPrintString(msg + msgIdx, qname.localName);
			msgIdx = strlen(msg);
		}
	}

	tmp_err_code = updateListLastAttribute(isAttribute, &(appD->outData), msg);
	if (tmp_err_code != EXIP_OK) {
		fprintf(stderr, "Unable to update list msg at sample_qnameData - error code: %d", tmp_err_code);
		return tmp_err_code;
	}
	return EXIP_OK;
}

// Stuff needed for the OUT_XML Output Format
// ******************************************
static void push(struct element **stack, struct element *el)
{
	if (*stack == NULL)
		*stack = el;
	else
	{
		el->next = *stack;
		*stack = el;
	}
}

static struct element *pop(struct element **stack)
{
	if (*stack == NULL)
		return NULL;
	else
	{
		struct element *result;
		result = *stack;
		*stack = (*stack)->next;
		return result;
	}
}

static struct element *createElement(char *name)
{
	struct element *el;
	el = malloc(sizeof(struct element));
	if (el == NULL)
		exit(1);
	el->next = NULL;
	el->name = malloc(strlen(name) + 1);
	if (el->name == NULL)
		exit(1);
	strcpy(el->name, name);
	return el;
}

static void destroyElement(struct element *el)
{
	free(el->name);
	free(el);
}
// ******************************************

static char lookupPrefix(struct appData *aData, String ns, unsigned char *prxHit, unsigned char *prefixIndex)
{
	int i;
	for (i = 0; i < aData->prefixesCount; i++)
	{
		if (stringEqualToAscii(ns, aData->prefixes[i]))
		{
			*prefixIndex = i;
			*prxHit = 1;
			return 0;
		}
	}

	if (aData->prefixesCount == MAX_PREFIXES)
		return 1;
	else
	{
		memcpy(aData->prefixes[aData->prefixesCount], ns.str, ns.length);
		aData->prefixes[aData->prefixesCount][ns.length] = '\0';
		*prefixIndex = aData->prefixesCount;
		aData->prefixesCount += 1;
		*prxHit = 0;
		return 0;
	}
}
