#include "codec_common.h"

size_t readFileInputStream(void *buf, size_t readSize, void *stream)
{
    FILE *infile = (FILE *)stream;
    return fread(buf, 1, readSize, infile);
}

size_t writeFileOutputStream(void *buf, size_t readSize, void *stream)
{
    FILE *outfile = (FILE *)stream;
    return fwrite(buf, 1, readSize, outfile);
}