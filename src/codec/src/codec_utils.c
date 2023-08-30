/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file codec_utils.c
 * @brief Decode and encode utility functions.
 */

#include "codec_utils.h"
#include "grammarGenerator.h"

#define MAX_XSD_FILES_COUNT 10 // up to 10 XSD files

List new_list() {
    List list = {.head = NULL, .tail = NULL, .size = 0};
    return list;
}

void push_back(List *list, void *data, size_t size) {
    Node *node = (Node*)calloc(1, sizeof(Node));
    node->data = calloc(1, size + 1);
    memcpy(node->data, data, size);
    node->size = size;
    node->next = NULL;

    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
}

Node* pop_front(List *list) {
    if (list->head == NULL) {
        return NULL;
    }

    Node *node = list->head;
    list->head = list->head->next;

    if (list->head == NULL) {
        list->tail = NULL;
    }

    list->size--;
    return node;
}

Node* pop_back(List *list) {
    return pop_at(list, list->size - 1);
}

Node* pop_at(List *list, int index) {
    if (index < 0 || index >= list->size) {
        return NULL;
    }

    Node *node = NULL;

    if (index == 0) {
        return pop_front(list);
    }

    Node *prev = list->head;
    for (int i = 0; i < index-1; i++) {
        prev = prev->next;
    }

    node = prev->next;
    prev->next = node->next;
    if (prev->next == NULL) {
        list->tail = prev;
    }

    list->size--;
    return node;
}

Node* getNth(List *list, int index) {
    if (index < 0 || index >= list->size) {
        return NULL;
    } else
    {
        if (index == 0) {
            return list->head;
        } else if (index == list->size-1) {
            return list->tail;
        } else {
            Node *currentNode = list->head;
            for (int i = 0; i < index; i++) {
                currentNode = currentNode->next;
            }
            return currentNode;
        }
    }
}

void print_list(List *list) {
    Node *node = list->head;
    printf("[");
    while (node != NULL) {
        printf("%.*s", (int)node->size, (char*)node->data);
        node = node->next;
        if (node != NULL) {
            printf(", ");
        }
    }
    printf("]\n");
}

void clear_node(Node *node)
{
    if (node != NULL)
        free(node->data);
    node->next = NULL;
    node->size = 0;
}

void delete_list(List *list) {
    Node *node = list->head;
    while (node != NULL) {
        Node *next = node->next;
        free(node->data);
        free(node);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

List* copy_list(List* list) {
    List* new_list = malloc(sizeof(List));
    if (!new_list) {
        return NULL;
    }
    new_list->size = list->size;
    new_list->head = NULL;
    new_list->tail = NULL;
    Node* current = list->head;
    while (current != NULL) {
        Node* new_node = malloc(sizeof(Node));
        if (!new_node) {
            delete_list(new_list);
            return NULL;
        }
        new_node->size = current->size;
        new_node->data = malloc(current->size);
        if (!new_node->data) {
            free(new_node);
            delete_list(new_list);
            return NULL;
        }
        memcpy(new_node->data, current->data, current->size);
        new_node->next = NULL;
        if (new_list->head == NULL) {
            new_list->head = new_node;
            new_list->tail = new_node;
        } else {
            new_list->tail->next = new_node;
            new_list->tail = new_node;
        }
        current = current->next;
    }
    return new_list;
}


EXIPSchema parseSchema(char *xsdList)
{
    errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
    FILE *schemaFile;
    EXIPSchema schema;
    BinaryBuffer buffer[MAX_XSD_FILES_COUNT]; // up to 10 XSD files
    char schemaFileName[500];
    unsigned int schemaFilesCount = 0;
    unsigned int i;
    char *token;

    for (token = strtok(xsdList, "=,"), i = 0; token != NULL; token = strtok(NULL, "=,"), i++)
    {
        schemaFilesCount++;
        if (schemaFilesCount > MAX_XSD_FILES_COUNT)
        {
            fprintf(stderr, "Too many xsd files given as an input: %d", schemaFilesCount);
            exit(1);
        }

        strcpy(schemaFileName, token);
        schemaFile = fopen(schemaFileName, "rb");
        if (!schemaFile)
        {
            fprintf(stderr, "Unable to open file %s", schemaFileName);
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
                fprintf(stderr, "Memory allocation error!");
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
    tmp_err_code = generateSchemaInformedGrammars(buffer, schemaFilesCount, SCHEMA_FORMAT_XSD_EXI, NULL, &schema, NULL);

    for (i = 0; i < schemaFilesCount; i++)
    {
        free(buffer[i].buf);
    }

    if (tmp_err_code != EXIP_OK)
    {
        printf("\nGrammar generation error occurred: %d", tmp_err_code);
        exit(1);
    }

    return schema;
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
        .drMap = (DatatypeRepresentationMap*)NULL,
        .blockSize = 0,
        .valueMaxLength = 0,
        .valuePartitionCapacity = 0,
        .user_defined_data = NULL,
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

void parseOpsMask(char* mask, EXIOptions* ops)
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