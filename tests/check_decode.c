/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file check_decode.c
 * @brief Tests the EXI decode module
 *
 * @date Sep 07, 2023
 * @author Vinicius
 * @version 0.1
 * @par[Revision] $Id$
 */

#include <stdlib.h>
#include <check.h>
#include "bodyDecode.h"
#include "decode.h"

#define MAX_PATH_LEN 200
#define BUFFER_LEN 1024

static char *dataDir;
static const char exiPath[] = "exip/subsGroups/root.exi";
static char* exiSchemaPath[2] = {"exip/subsGroups/root-xsd.exi","exip/subsGroups/sub-xsd.exi"};

static char* prependMultiPath(char** xsdList, int count, char *prependStr);

/* BEGIN: decode tests */

List defaultRootExi()
{
	List root = newList();
	pushBack(&root, (void *)"SD", 2);
	pushBack(&root, (void *)"SE http://exip.sourceforge.net/ one", 35);
	pushBack(&root, (void *)"AT attr=\"---\"", 13);
	pushBack(&root, (void*)"AT http://www.w3.org/2001/XMLSchema-instance schemaLocation=\"http://exip.sourceforge.net/ root.xsd \"", 100);
	pushBack(&root, (void*)"AT subs=\"7E-2\"", 14);
	pushBack(&root, (void*)"SE http://test.org/sub sallad", 29);
	pushBack(&root, (void*)"SE http://test.org/sub apple", 28);
	pushBack(&root, (void*)"AT http://www.w3.org/2001/XMLSchema-instance nil=\"true\"", 55);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub orange", 29);
	pushBack(&root, (void*)"CH Name", 7);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub grapes", 29);
	pushBack(&root, (void*)"0F00", 2);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub apple", 28);
	pushBack(&root, (void*)"CH true", 7);//16
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub orange", 29);
	pushBack(&root, (void*)"CH Name2", 8);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub grapes", 29);
	pushBack(&root, (void*)"0F01", 2);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub apple", 28);
	pushBack(&root, (void*)"CH false", 8);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub orange", 29);
	pushBack(&root, (void*)"CH Name3", 8);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://test.org/sub grapes", 29);
	pushBack(&root, (void*)"0F02", 2);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"SE http://exip.sourceforge.net/ piece", 37);
	pushBack(&root, (void*)"CH root:piece", 13);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"EE", 2);
	pushBack(&root, (void*)"ED", 2);
	return root;
}

START_TEST (test_decodeFromBuffer)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	List decodedData = newList();
	List defaultRoot = defaultRootExi();
	boolean hasOptions = FALSE;
	EXIOptions *options = NULL;
	char exiFullPath[MAX_PATH_LEN + strlen(exiPath)];
	char *exiSchemaFullPath;
	size_t pathlen;
	FILE *inFile;
	size_t inFileSize;
	char buf[BUFFER_LEN];

	pathlen = strlen(dataDir);
	memcpy(exiFullPath, dataDir, pathlen);
	exiFullPath[pathlen] = '/';
	memcpy(&exiFullPath[pathlen+1], exiPath, strlen(exiPath)+1);

	exiSchemaFullPath = prependMultiPath(exiSchemaPath, 2, dataDir);

	inFile = fopen(exiFullPath, "rb" );
    ck_assert_msg (inFile, "test_decodeFromBuffer couldn't open EXI file at %s\n", exiFullPath);

    inFileSize = fread(buf, 1, BUFFER_LEN, inFile);
	ck_assert_msg (inFileSize > 0, "test_decodeFromBuffer couldn't read EXI file at %s\n", exiFullPath);
	ck_assert_msg (inFileSize != BUFFER_LEN, "test_decodeFromBuffer EXI file is too big. Please increase the BUFFER_LEN size\n");

    fclose(inFile);

	tmp_err_code = decodeFromBuffer(exiSchemaFullPath, OUT_EXI, hasOptions, options, (void *)buf, inFileSize, &decodedData);
	ck_assert_msg (tmp_err_code == EXIP_OK, "decodeFromFile returns an error code %d\n", tmp_err_code);
	ck_assert_msg (cmpStrList(&decodedData, &defaultRoot), "decodeFromFile decoded file does not match expected data\n");
    free(exiSchemaFullPath);
	deleteList(&decodedData);
}
END_TEST

START_TEST (test_decodeFromFile)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	List decodedData = newList();
	List defaultRoot = defaultRootExi();
	boolean hasOptions = FALSE;
	EXIOptions *options = NULL;
	char exiFullPath[MAX_PATH_LEN + strlen(exiPath)];
	char *exiSchemaFullPath;
	size_t pathlen;
	FILE *inFile;
	size_t inFileSize;
	char buf[BUFFER_LEN];

	pathlen = strlen(dataDir);
	memcpy(exiFullPath, dataDir, pathlen);
	exiFullPath[pathlen] = '/';
	memcpy(&exiFullPath[pathlen+1], exiPath, strlen(exiPath)+1);

	exiSchemaFullPath = prependMultiPath(exiSchemaPath, 2, dataDir);

	inFile = fopen(exiFullPath, "rb" );
    ck_assert_msg (inFile, "test_decodeFromBuffer couldn't open EXI file at %s\n", exiFullPath);

	tmp_err_code = decodeFromFile(exiSchemaFullPath, OUT_EXI, hasOptions, options, exiFullPath, &decodedData);
	ck_assert_msg (tmp_err_code == EXIP_OK, "decodeFromFile returns an error code %d\n", tmp_err_code);
	ck_assert_msg (cmpStrList(&decodedData, &defaultRoot), "decodeFromFile decoded file does not match expected data\n");
    free(exiSchemaFullPath);
	deleteList(&decodedData);
}
END_TEST
/* END: decode tests */

static char* prependMultiPath(char** xsdList, int count, char *prependStr)
{
	FILE *schemaFile;
	size_t pathlen = strlen(prependStr);
	const size_t MAX_TOTAL_PATH_LEN = MAX_PATH_LEN*count + strlen(xsdList[0])*count;
	char *exiPath = calloc(1, MAX_TOTAL_PATH_LEN);
    if (!exiPath) 
    {
        return NULL;
    }
	char *pathPtr = exiPath;

	for (int i = 0; i < count; i++)
	{
		memcpy(pathPtr, prependStr, pathlen);
		pathPtr[pathlen] = '/';
		memcpy(&pathPtr[pathlen+1], xsdList[i], strlen(xsdList[i])+1);
		schemaFile = fopen(pathPtr, "rb" );
		if(!schemaFile)
		{
            free(exiPath);
			ck_abort_msg("Unable to open file %s", pathPtr);
			return NULL;
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

    return exiPath;
}

Suite * decode_suite (void)
{
  Suite *s = suite_create ("Decode");

  {
	  /* Decode test case */
	  TCase *tc_decode = tcase_create ("Decode");
	  tcase_add_test (tc_decode, test_decodeFromBuffer);
	  tcase_add_test (tc_decode, test_decodeFromFile);
	  suite_add_tcase (s, tc_decode);
  }

  return s;
}

int main (int argc, char *argv[])
{
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

	int number_failed;
	Suite *s = decode_suite();
	SRunner *sr = srunner_create (s);
#ifdef _MSC_VER
	srunner_set_fork_status(sr, CK_NOFORK);
#endif
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
