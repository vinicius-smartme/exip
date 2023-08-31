#include "decode.h"
#include "singleLinkedList.h"
#include "exipConfig.h"

#define BUFFER_LEN 1024
int main () {
    char exi_path[] = "../examples/simpleDecoding/exipd-test-schema.exi";
    char schema_path[] = "../examples/simpleDecoding/exipd-test-schema-xsd.exi";
    // char exi_path[] = "../examples/simpleDecoding/exipd-test.exi";
    //char * schema_path = NULL;
    char xml_path[] = "test_encode.xml";
    unsigned char outFlag = OUT_EXI;
    boolean outOfBandOpts = FALSE;
    EXIOptions *opts = NULL;
    char buf[BUFFER_LEN];
    FILE *infile;
    size_t inFileSize;
    List output_file = newList();
    List output_buffer = newList();
    
    printf("\n ------------------- Decode from file ------------------- \n");
    infile = fopen(exi_path, "rb" );
    if(!infile)
    {
        printf("\nUnable to open file %s\n", exi_path);
        exit(1);
    }

    decodeFromFile(schema_path, outFlag, outOfBandOpts, opts, (void *)infile, &output_file);
    fclose(infile);
    printf("\n ------------------- List: \n");
    printList(&output_file);
    deleteList(&output_file);

    printf("\n ------------------- Decode from buffer ------------------- \n");
    infile = fopen(exi_path, "rb" );
    if(!infile)
    {
        printf("\nUnable to open file %s\n", exi_path);
        exit(1);
    }
    inFileSize = fread(buf, 1, BUFFER_LEN, infile);
    if(fread <= 0)
    {
        fprintf(stderr, "\nUnable to read file %s\n", exi_path);
        exit(1);
    } else if (inFileSize == BUFFER_LEN) {
		printf("Warning: Maybe file size is bigger than the buffer !!\n");
    }
    printf("Read %d(Buffer size: %d)\n", inFileSize, BUFFER_LEN);
    fclose(infile);

    decodeFromBuffer(schema_path, outFlag, outOfBandOpts, opts, (void *)buf, inFileSize, &output_buffer);
    printf("\n ------------------- List: \n");
    printList(&output_buffer);
    //deleteList(&output_buffer);

    printf("\n ------------------- Encode from buffer ------------------- \n");

    output_file = newList();
    memset(buf, 0, BUFFER_LEN);
    encodeFromBuffer(schema_path, outFlag, outOfBandOpts, opts, &output_buffer, output_buffer.size, buf, BUFFER_LEN);
    printf("\n ------------------- List: \n");
    printList(&output_file);
    deleteList(&output_file);

    deleteList(&output_buffer);

    return 0;
}


// #include "EXIParser.h"
// #include "EXISerializer.h"
// #include "stringManipulate.h"
// #include "../src/stringTables/include/sTables.h"
// #include "../src/common/include/memManagement.h"
// #include "../src/common/include/dynamicArray.h"

// #define OUTPUT_BUFFER_SIZE 2000
// void fail_unless (boolean cond, const char* msg, errorCode tmp_err_code) {
//     if (!cond) {
//         fprintf(stderr, "Error: %s (error code: %d\n)", msg, tmp_err_code);
//         exit(EXIT_FAILURE);
//     }
// }

// int main () {
//     EXIStream testStrm;
// 	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
// 	String testStr = {"TEST-007", 8};

// 	// IV: Initialize the stream
// 	{
// 		tmp_err_code = initAllocList(&(testStrm.memList));

// 		testStrm.context.bitPointer = 0;
// 		testStrm.buffer.bufLen = 0;
// 		testStrm.buffer.bufContent = 0;
// 		testStrm.buffer.bufStrm = EMPTY_BUFFER_STREAM;
// 		tmp_err_code += createValueTable(&testStrm.valueTable);
// 		testStrm.schema = memManagedAllocate(&testStrm.memList, sizeof(EXIPSchema));
// 		fail_unless (testStrm.schema != NULL, "Memory alloc error", 0);
// 		/* Create and initialize initial string table entries */
// 		tmp_err_code += createDynArray(&testStrm.schema->uriTable.dynArray, sizeof(UriEntry), DEFAULT_URI_ENTRIES_NUMBER);
// 		tmp_err_code += createUriTableEntries(&testStrm.schema->uriTable, FALSE);
// 	}
// 	fail_unless (tmp_err_code == EXIP_OK, "initStream returns an error code %d", tmp_err_code);

// 	testStrm.gStack->currQNameID.uriId = 1; // http://www.w3.org/XML/1998/namespace
// 	testStrm.gStack->currQNameID.lnId = 2; // lang

// 	tmp_err_code = addValueEntry(&testStrm, testStr, testStrm.gStack->currQNameID);

// 	fail_unless (tmp_err_code == EXIP_OK, "addValueEntry returns an error code %d", tmp_err_code);
// #if VALUE_CROSSTABLE_USE
// 	fail_unless (testStrm.schema->uriTable.uri[testStrm.gStack->currQNameID.uriId].lnTable.ln[testStrm.gStack->currQNameID.lnId].vxTable != NULL, "addValueEntry does not create vxTable", 0);
// 	fail_unless (testStrm.schema->uriTable.uri[testStrm.gStack->currQNameID.uriId].lnTable.ln[testStrm.gStack->currQNameID.lnId].vxTable->count == 1, "addValueEntry does not create correct vxTable", 0);
// #endif
// 	fail_unless (testStrm.valueTable.count == 1, "addValueEntry does not create global value entry", 0);

// 	destroyDynArray(&testStrm.valueTable.dynArray);
// 	destroyDynArray(&testStrm.schema->uriTable.dynArray);
// 	freeAllocList(&testStrm.memList);
// }