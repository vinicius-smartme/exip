/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file encode.h
 * @brief Interface for a function encoding a sample EXI message
 */

#ifndef ENCODE_H_
#define ENCODE_H_

#include "procTypes.h"

errorCode encode_from_file(EXIPSchema *schemaPtr, void *out_stream);
errorCode encode_from_buffer(EXIPSchema *schemaPtr, void *out_stream);

#endif /* ENCODE_H_ */