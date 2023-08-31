#include "decode.h"
#include "codec_utils.h"
#include "exipConfig.h"

#define BUFFER_LEN 1024
int main () {
    char exi_path[] = "../examples/simpleDecoding/exipd-test-schema.exi";
    char schema_path[] = "../examples/simpleDecoding/exipd-test-schema-xsd.exi";
    // char exi_path[] = "../examples/simpleDecoding/exipd-test.exi";
    //char * schema_path = NULL;
    char xml_path[] = "test_encode.xml";
    EXIPSchema schema = parseSchema(schema_path);
    unsigned char outFlag = OUT_EXI;
    boolean outOfBandOpts = FALSE;
    EXIOptions *opts = NULL;
    char buf[BUFFER_LEN];
    FILE *infile;
    size_t inFileSize;
    List output_file = new_list();
    List output_buffer = new_list();
    
    printf("\n ------------------- Decode from file ------------------- \n");
    infile = fopen(exi_path, "rb" );
    if(!infile)
    {
        printf("\nUnable to open file %s\n", exi_path);
        exit(1);
    }

    decode_from_file(&schema, outFlag, outOfBandOpts, opts, (void *)infile, &output_file);
    fclose(infile);
    printf("\n ------------------- List: \n");
    print_list(&output_file);
    delete_list(&output_file);

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

    decode_from_buffer(&schema, outFlag, outOfBandOpts, opts, (void *)buf, inFileSize, &output_buffer);
    printf("\n ------------------- List: \n");
    print_list(&output_buffer);
    //delete_list(&output_buffer);

    printf("\n ------------------- Encode from buffer ------------------- \n");

    output_file = new_list();
    memset(buf, 0, BUFFER_LEN);
    encode_from_buffer(&schema, outFlag, outOfBandOpts, opts, &output_buffer, output_buffer.size, buf, BUFFER_LEN);
    printf("\n ------------------- List: \n");
    print_list(&output_file);
    delete_list(&output_file);

    delete_list(&output_buffer);

    return 0;
}
