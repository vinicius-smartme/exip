#ifndef SCHEMAIDMODE_H
#define SCHEMAIDMODE_H

/**
 * @name Handling of SchemaID header field
 *
 * SchemaID option modes:
 * @def SCHEMA_ID_ABSENT
 * 		default,  no statement is made about the schema information
 * @def SCHEMA_ID_SET
 * 		some sting identification of the schema is given
 * @def SCHEMA_ID_NIL
 * 		no schema information is used for processing the EXI body (i.e. a schema-less EXI stream)
 * @def SCHEMA_ID_EMPTY
 * 		no user defined schema information is used for processing the EXI body; however, the built-in XML schema types are available for use in the EXI body
 *
 * @see http://www.w3.org/TR/2011/REC-exi-20110310/#key-schemaIdOption
 */
enum SchemaIdMode
{
	SCHEMA_ID_ABSENT = 0,
	SCHEMA_ID_SET    = 1,
	SCHEMA_ID_NIL    = 2,
	SCHEMA_ID_EMPTY  = 3
};

typedef enum SchemaIdMode SchemaIdMode;

#endif //SCHEMAIDMODE_H