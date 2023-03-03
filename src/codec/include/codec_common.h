/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file codec_common.h
 * @brief Decode and encode common functions.
 */

#ifndef CODE_COMMON_H_
#define CODE_COMMON_H_

#include "procTypes.h"

size_t readFileInputStream(void* buf, size_t readSize, void* stream);
size_t readBufferInputStream(void* buf, size_t readSize, void* stream);

size_t writeFileOutputStream(void* buf, size_t readSize, void* stream);
size_t writeBufferOutputStream(void* buf, size_t readSize, void* stream);

#endif /* CODE_COMMON_H_ */