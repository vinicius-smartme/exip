#include "parseSchema.h"
#include "grammarGenerator.h"

errorCode parseSchema(char *xsdList, EXIPSchema *schema)
{
    errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
    FILE *schemaFile;
    BinaryBuffer buffer[MAX_XSD_FILES_COUNT]; // up to 10 XSD files
    char schemaFileName[500];
    unsigned int schemaFilesCount = 0;
    unsigned int i;
    char *token;

    if (!xsdList || !schema)
    {
        return EXIP_INVALID_INPUT;
    }

    for (token = strtok(xsdList, "=,"), i = 0; token != NULL; token = strtok(NULL, "=,"), i++)
    {
        schemaFilesCount++;
        if (schemaFilesCount > MAX_XSD_FILES_COUNT)
        {
            fprintf(stderr, "Too many xsd files given as an input: %d\n", schemaFilesCount);
            exit(1);
        }

        strcpy(schemaFileName, token);
        schemaFile = fopen(schemaFileName, "rb");
        if (!schemaFile)
        {
            fprintf(stderr, "Unable to open XSD file \"%s\" for parsing\n", schemaFileName);
            exit(1);
        }
        else
        {
            // Get file length
            fseek(schemaFile, 0, SEEK_END);
            buffer[i].bufLen = ftell(schemaFile) + 1;
            fseek(schemaFile, 0, SEEK_SET);

            // Allocate memory
            buffer[i].buf = (char *)malloc(buffer[i].bufLen);
            if (!buffer[i].buf)
            {
                fprintf(stderr, "Memory allocation error!\n");
                fclose(schemaFile);
                exit(1);
            }

            // Read file contents into buffer
            fread(buffer[i].buf, buffer[i].bufLen, 1, schemaFile);
            fclose(schemaFile);

            buffer[i].bufContent = buffer[i].bufLen;
            buffer[i].ioStrm.readWriteToStream = NULL;
            buffer[i].ioStrm.stream = NULL;
            buffer[i].bufStrm = EMPTY_BUFFER_STREAM;
        }
    }

    // Generate the EXI grammars based on the schema information
    tmp_err_code = generateSchemaInformedGrammars(buffer, schemaFilesCount, SCHEMA_FORMAT_XSD_EXI, NULL, schema, NULL);

    for (i = 0; i < schemaFilesCount; i++)
    {
        free(buffer[i].buf);
    }

    if (tmp_err_code != EXIP_OK)
    {
        printf("\nGrammar generation error occurred: %d\n", tmp_err_code);
    }

    return tmp_err_code;
}

EXIOptions setOpsMask(
    char preservation_opt_flags,
    boolean strict_schema,
    boolean fragments,
    boolean self_contained,
    boolean compression,
    boolean pre_compression,
    boolean alignment,
    size_t valuePartitionCapacity,
    size_t valueMaxLength,
    uint32_t blockSize)
{
    EXIOptions ops = {
        .preserve = preservation_opt_flags,
        .schemaIDMode = SCHEMA_ID_ABSENT,
        .schemaID = EMPTY_STRING,
        .drMap = (DatatypeRepresentationMap *)NULL,
        .blockSize = 0,
        .valueMaxLength = 0,
        .valuePartitionCapacity = 0,
    };

    if (strict_schema)
    {
        SET_STRICT(ops.enumOpt);
    }
    if (fragments)
    {
        SET_FRAGMENT(ops.enumOpt);
    }
    if (self_contained)
    {
        SET_SELF_CONTAINED(ops.enumOpt);
    }
    if (compression)
    {
        SET_COMPRESSION(ops.enumOpt);
    }
    if (pre_compression)
    {
        SET_ALIGNMENT(ops.enumOpt, PRE_COMPRESSION);
    }
    if (alignment)
    {
        SET_ALIGNMENT(ops.enumOpt, BYTE_ALIGNMENT);
    }

    if (valuePartitionCapacity > 0)
    {
        ops.valuePartitionCapacity = (Index)valuePartitionCapacity;
    }

    if (valueMaxLength > 0)
    {
        ops.valueMaxLength = (Index)valueMaxLength;
    }

    if (blockSize > 0)
    {
        ops.blockSize = blockSize;
    }

    return ops;
}

void parseOpsMask(char *mask, EXIOptions *ops)
{
    unsigned int i;
    char *token;

    for (token = strtok(mask, "=%"), i = 0; token != NULL; token = strtok(NULL, "=%"), i++)
    {
        switch (i)
        {
        case 0:
            if (strcmp(token, "-"))
            {
                // Preservation Options: c - comments, d - dtds, l - lexicalvalues, p - pis, x- prefixes
                if (strstr(token, "c") != NULL)
                    SET_PRESERVED(ops->preserve, PRESERVE_COMMENTS);
                if (strstr(token, "d") != NULL)
                    SET_PRESERVED(ops->preserve, PRESERVE_DTD);
                if (strstr(token, "l") != NULL)
                    SET_PRESERVED(ops->preserve, PRESERVE_LEXVALUES);
                if (strstr(token, "p") != NULL)
                    SET_PRESERVED(ops->preserve, PRESERVE_PIS);
                if (strstr(token, "x") != NULL)
                    SET_PRESERVED(ops->preserve, PRESERVE_PREFIXES);
            }
            break;
        case 1:
            if (strcmp(token, "-"))
            {
                // Other options: v - strict interpretation of schema, f - fragments
                // r - selfContained, c - compression, p - pre-compression, a - aligned to bytes\n");
                if (strstr(token, "v") != NULL)
                    SET_STRICT(ops->enumOpt);
                if (strstr(token, "f") != NULL)
                    SET_FRAGMENT(ops->enumOpt);
                if (strstr(token, "r") != NULL)
                    SET_SELF_CONTAINED(ops->enumOpt);
                if (strstr(token, "c") != NULL)
                    SET_COMPRESSION(ops->enumOpt);
                if (strstr(token, "p") != NULL)
                    SET_ALIGNMENT(ops->enumOpt, PRE_COMPRESSION);
                else if (strstr(token, "a") != NULL)
                    SET_ALIGNMENT(ops->enumOpt, BYTE_ALIGNMENT);
            }
            break;
        case 2:
            if (strcmp(token, "-"))
            {
                // valuePartitionCapacity
                ops->valuePartitionCapacity = (Index)strtol(token, NULL, 10);
            }
            break;
        case 3:
            if (strcmp(token, "-"))
            {
                // valueMaxLength
                ops->valueMaxLength = (Index)strtol(token, NULL, 10);
            }
            break;
        case 4:
            if (strcmp(token, "-"))
            {
                // blockSize
                ops->blockSize = (uint32_t)strtol(token, NULL, 10);
            }
            break;
        default:
        {
            fprintf(stderr, "Wrong options mask: %s\n", mask);
            exit(1);
        }
        }
    }
}