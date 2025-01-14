/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file exipd.c
 * @brief Command-line utility
 *
 * @date Nov 5, 2012
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "decodeBufferTestEXI.h"
#include "grammarGenerator.h"
#include "parseSchema.h"

#define MAX_XSD_FILES_COUNT 10 // up to 10 XSD files

static void printfHelp();
static void parseOpsMask(char* mask, EXIOptions* ops);

size_t readFileInputStream(void* buf, size_t readSize, void* stream);

#define IN_DATA_SIZE 1024

int main(int argc, char *argv[])
{
	FILE *infile = stdin;
	char sourceFileName[500];
	EXIPSchema schema;
	EXIPSchema* schemaPtr = NULL;
	EXIOptions ops;
	EXIOptions* opsPtr = NULL;
	boolean outOfBandOpts = FALSE;
	unsigned char outFlag = OUT_EXI; // Default output option
	unsigned int argIndex = 1;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	char buf[IN_DATA_SIZE];
	size_t inFileSize;

	strcpy(sourceFileName, "stdin");
	makeDefaultOpts(&ops);

	if(argc == 0)
	{
		printfHelp();
		return 0;
	}
	else if(argc > 1)
	{
		if(strcmp(argv[argIndex], "-help") == 0)
		{
			printfHelp();
			return 0;
		}
		else if(strcmp(argv[argIndex], "-exi") == 0)
		{
			outFlag = OUT_EXI;
			if(argc == 2)
			{
				printfHelp();
				return 0;
			}

			argIndex += 1;
		}
		else if(strcmp(argv[argIndex], "-xml") == 0)
		{
			outFlag = OUT_XML;
			if(argc == 2)
			{
				printfHelp();
				return 0;
			}

			argIndex += 1;
		}

		if(strstr(argv[argIndex], "-ops") != NULL)
		{
			char *mask = argv[argIndex] + 4;
			outOfBandOpts = TRUE;

			parseOpsMask(mask, &ops);
			opsPtr = &ops;

			argIndex += 1;
		}

		if(strstr(argv[argIndex], "-schema") != NULL)
		{
			char *xsdList = argv[argIndex] + 7;

			tmp_err_code = parseSchema(xsdList, NULL, &schema);
			if (tmp_err_code != EXIP_OK) {
				fprintf(stderr, "Unable to parse schema: error %d\n", tmp_err_code);
				exit(1);
			}
			schemaPtr = &schema;

			argIndex += 1;
		}
	}

	if(argIndex < argc)
	{
		strcpy(sourceFileName, argv[argIndex]);

		infile = fopen(sourceFileName, "rb" );
		if(!infile)
		{
			fprintf(stderr, "Unable to open file %s\n", sourceFileName);
			exit(1);
		}
	}

    inFileSize = fread(buf, 1, IN_DATA_SIZE, infile);
    if(fread <= 0)
    {
        fprintf(stderr, "\nUnable to read file %s\n", sourceFileName);
        exit(1);
    } else if (inFileSize == IN_DATA_SIZE) {
		printf("Warning: Maybe file size is bigger than the buffer !!\n");
	}
	printf("Read %ld(Buffer size: %d)\n", inFileSize, IN_DATA_SIZE);

	tmp_err_code = decode(schemaPtr, outFlag, NULL, buf, inFileSize, outOfBandOpts, opsPtr, NULL);

	if(schemaPtr != NULL)
		destroySchema(schemaPtr);
	fclose(infile);

	if(tmp_err_code != EXIP_OK)
	{
		printf("\nError (code: %d) during parsing of the EXI stream: %s\n", tmp_err_code, sourceFileName);
		return 1;
	}
	else
	{
		printf("\nSuccessful parsing of the EXI stream: %s\n", sourceFileName);
		return 0;
	}
}

static void printfHelp()
{
    printf("\n" );
    printf("  EXIP     Copyright (c) 2010 - 2012, EISLAB - Luleå University of Technology Version 0.4 \n");
    printf("           Author: Rumen Kyusakov\n");
    printf("  Usage:   exipd [options] [exi_in]\n\n");
    printf("           Options: [-help | [ -xml | -exi ] [-ops=<ops_mask>] -schema=<xsd_in>] \n");
    printf("           -schema :   uses schema defined in <xsd_in> for decoding. All referenced schema files should be included in <xsd_in>\n");
    printf("           <xsd_in>:   Comma-separated list of schema documents encoded in EXI with Preserve.prefixes. The first schema is the\n");
    printf("                       main one and the rest are schemas that are referenced from the main one through the <xs:import> statement.\n");
    printf("           ops_mask:   Specify out-of-band options used for encoding. Fields are delimited by %%.\n");
    printf("                       Use this argument only for specifying out-of-band options. That is if no options are specified in the header of the <exi_in>\n");
    printf("                       The format is: <1>%%<2>%%<3>%%<4>%%<5> where:\n");
    printf("                       <1> - Preservation Options: c - comments, d - dtds, l - lexicalvalues, p - pis, x- prefixes. If none set then \"-\" \n");
    printf("                       <2> - Other options: v - strict interpretation of schema, f - fragments\n");
    printf("                       r - selfContained, c - compression, p - pre-compression, a - aligned to bytes. If none set then \"-\"\n");
    printf("                       <3> - valuePartitionCapacity. If not set then \"-\"\n");
    printf("                       <4> - valueMaxLength. If not set then \"-\"\n");
    printf("                       <5> - blockSize. If not set then \"-\"\n");
    printf("                       For example: cx%%v%%0%%-%%- sets the folowing options: Preservation of comments and prefixes, strict schema informed\n");
    printf("                       and valuePartitionCapacity = 0\n");
    printf("           -exi    :   EXI formated output [default]\n");
    printf("           -xml    :   XML formated output\n");
    printf("           -help   :   Prints this help message\n\n");
    printf("           exi_in  :   input file containing the EXI stream (stdin if none specified)\n\n");
    printf("  Purpose: This program tests the EXIP decoding functionality. The result is printed on the stdout.\n");
    printf("\n" );
}

size_t readFileInputStream(void* buf, size_t readSize, void* stream)
{
	FILE *infile = (FILE*) stream;
	return fread(buf, 1, readSize, infile);
}

static void parseOpsMask(char* mask, EXIOptions* ops)
{
	unsigned int i;
	char *token;

	for (token = strtok(mask, "=%"), i = 0; token != NULL; token = strtok(NULL, "=%"), i++)
	{
		switch(i)
		{
			case 0:
			if(strcmp(token, "-"))
			{
				// Preservation Options: c - comments, d - dtds, l - lexicalvalues, p - pis, x- prefixes
				if(strstr(token, "c") != NULL)
					SET_PRESERVED(ops->preserve, PRESERVE_COMMENTS);
				if(strstr(token, "d") != NULL)
					SET_PRESERVED(ops->preserve, PRESERVE_DTD);
				if(strstr(token, "l") != NULL)
					SET_PRESERVED(ops->preserve, PRESERVE_LEXVALUES);
				if(strstr(token, "p") != NULL)
					SET_PRESERVED(ops->preserve, PRESERVE_PIS);
				if(strstr(token, "x") != NULL)
					SET_PRESERVED(ops->preserve, PRESERVE_PREFIXES);
			}
			break;
			case 1:
			if(strcmp(token, "-"))
			{
				// Other options: v - strict interpretation of schema, f - fragments
			    // r - selfContained, c - compression, p - pre-compression, a - aligned to bytes\n");
				if(strstr(token, "v") != NULL)
					SET_STRICT(ops->enumOpt);
				if(strstr(token, "f") != NULL)
					SET_FRAGMENT(ops->enumOpt);
				if(strstr(token, "r") != NULL)
					SET_SELF_CONTAINED(ops->enumOpt);
				if(strstr(token, "c") != NULL)
					SET_COMPRESSION(ops->enumOpt);
				if(strstr(token, "p") != NULL)
					SET_ALIGNMENT(ops->enumOpt, PRE_COMPRESSION);
				else if(strstr(token, "a") != NULL)
					SET_ALIGNMENT(ops->enumOpt, BYTE_ALIGNMENT);
			}
			break;
			case 2:
			if(strcmp(token, "-"))
			{
				// valuePartitionCapacity
				ops->valuePartitionCapacity = (Index) strtol(token, NULL, 10);
			}
			break;
			case 3:
			if(strcmp(token, "-"))
			{
				// valueMaxLength
				ops->valueMaxLength = (Index) strtol(token, NULL, 10);
			}
			break;
			case 4:
			if(strcmp(token, "-"))
			{
				// blockSize
				ops->blockSize = (uint32_t) strtol(token, NULL, 10);
			}
			break;
			default:
			{
				fprintf(stderr, "Wrong options mask: %s", mask);
				exit(1);
			}
		}
	}
}
