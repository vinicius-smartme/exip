/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file codec_utils.h
 * @brief Decode and encode utility functions.
 */

#ifndef CODE_UTILS_H_
#define CODE_UTILS_H_

#include "procTypes.h"

EXIPSchema parseSchema(char *xsdList);
/*void parseOpsMask(char* mask, EXIOptions* ops);
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
    uint32_t blockSize);*/

#endif /* CODE_UTILS_H_ */