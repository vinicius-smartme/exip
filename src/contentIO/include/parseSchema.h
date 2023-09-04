#ifndef PARSE_SCHEMA_H
#define PARSE_SCHEMA_H

#include "procTypes.h"

#define MAX_XSD_FILES_COUNT 10 // up to 10 XSD files

/**
 * @brief Parses a list of schemas on the path xsdList into an EXIPSchema object
 * 
 *
 * @param[in] xsdList a list of paths to xsd schemas
 * @param[in] maskOpt EXIOptions if any, otherwise NULL
 * @param[out] schema the parsed EXIPSchema object
 * @return Error handling code
 */
errorCode parseSchema(const char *xsdList, const EXIOptions *maskOpt, EXIPSchema *schema);


#endif //PARSE_SCHEMA_H