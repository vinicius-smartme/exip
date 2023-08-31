#ifndef EXIOPTIONS_H
#define EXIOPTIONS_H

#include "EXIPrimitives.h"
#include "schemaIdMode.h"

struct DatatypeRepresentationMap
{
	void* TODO; //TODO: fill in the information for this structure
};

typedef struct DatatypeRepresentationMap DatatypeRepresentationMap;

struct EXIOptions
{
	/**
	 * Use the macros GET_ALIGNMENT(p), WITH_COMPRESSION(p), WITH_STRICT,
	 * WITH_FRAGMENT(p), WITH_SELF_CONTAINED(p) to extract the options:
	 * alignment, compression, strict, fragment and selfContained
	 *
	 * @see options_defs
	 */
	unsigned char enumOpt;

	/**
	 * Specifies whether comments, pis, etc. are preserved - bit mask of booleans
	 * Use IS_PRESERVED macro to retrieve the values different preserve options
	 */
	unsigned char preserve;

	/** schemaID mode, default SCHEMA_ID_ABSENT */
	SchemaIdMode schemaIDMode;

	/**
	 * Identify the schema information, if any, used to encode the body.
	 * It the schemaID field is absent or empty, then schemaID is
	 * an empty string. Use schemaIDMode to check/set the exact schemaID mode
	 * of operation
	 */
	String schemaID;

	/**
	 * Specify alternate datatype representations for typed values in the EXI body
	 */
	DatatypeRepresentationMap* drMap;

	/**
	 *  Specifies the block size used for EXI compression
	 */
	uint32_t blockSize;

	/**
	 * Specifies the maximum string length of value content items to be considered for addition to the string table.
	 * INDEX_MAX - unbounded
	 */
	Index valueMaxLength;

	/**
	 * Specifies the total capacity of value partitions in a string table
	 * INDEX_MAX - unbounded
	 */
	Index valuePartitionCapacity;

	/**
	 * User defined meta-data may be added
	 */
	void* user_defined_data;
};

typedef struct EXIOptions EXIOptions;

#endif //EXIOPTIONS_H