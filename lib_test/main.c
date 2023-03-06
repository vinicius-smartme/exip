#include "decode.h"
#include "codec_utils.h"
#include "exipConfig.h"

#define BUFFER_LEN 1024
int main () {
    char exi_path[] = "../examples/simpleDecoding/exipd-test-schema.exi";
    char schema_path[] = "../examples/simpleDecoding/exipd-test-schema-xsd.exi";
    EXIPSchema schema = parseSchema(schema_path);
    unsigned char outFlag = OUT_EXI;
    boolean outOfBandOpts = FALSE;
    EXIOptions *opts = NULL;
    char buf[BUFFER_LEN];
    FILE *infile;
    size_t inFileSize;

    infile = fopen(exi_path, "rb" );
    if(!infile)
    {
        printf("\nUnable to open file %s\n", exi_path);
        exit(1);
    }

    printf("\n ------------------- Decode from file ------------------- \n");
    decode_from_file(&schema, outFlag, (void *)infile, outOfBandOpts, opts);
    fclose(infile);

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
    
    decode_from_buffer(&schema, outFlag, (void *)buf, inFileSize, outOfBandOpts, opts);
    return 0;
}