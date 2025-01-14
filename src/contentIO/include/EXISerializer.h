/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file EXISerializer.h
 * @brief Interface for serializing an EXI stream
 * Application will use this interface to work with the EXIP serializer
 *
 * @date Sep 30, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#ifndef EXISERIALIZER_H_
#define EXISERIALIZER_H_

#include "procTypes.h"

struct EXISerializer
{
	// For handling the meta-data (document structure)
	errorCode (*startDocument)(EXIStream* strm);
	errorCode (*endDocument)(EXIStream* strm);
	errorCode (*startElement)(EXIStream* strm, QName qname, EXITypeClass* valueType);
	errorCode (*endElement)(EXIStream* strm);
	errorCode (*attribute)(EXIStream* strm, QName qname, boolean isSchemaType, EXITypeClass* valueType);

	// For handling the data
	errorCode (*intData)(EXIStream* strm, Integer int_val);
	errorCode (*booleanData)(EXIStream* strm, boolean bool_val);
	errorCode (*stringData)(EXIStream* strm, const String str_val);
	errorCode (*floatData)(EXIStream* strm, Float float_val);
	errorCode (*binaryData)(EXIStream* strm, const char* binary_val, Index nbytes);
	errorCode (*dateTimeData)(EXIStream* strm, EXIPDateTime dt_val);
	errorCode (*decimalData)(EXIStream* strm, Decimal dec_val);
	errorCode (*listData)(EXIStream* strm, unsigned int itemCount);
	errorCode (*qnameData)(EXIStream* strm, QName qname); // xsi:type value only

	// Miscellaneous
	errorCode (*processingInstruction)(EXIStream* strm, const String name, const String text);
	errorCode (*comment)(EXIStream* strm, const String comment);
	errorCode (*namespaceDeclaration)(EXIStream* strm, const String ns, const String prefix, boolean isLocalElementNS);
	errorCode (*docType)(EXIStream* strm, boolean public, const String system, const String text);
	errorCode (*entityReference)(EXIStream* strm, const String reference);

	// EXI specific
	errorCode (*exiHeader)(EXIStream* strm);
	errorCode (*selfContained)(EXIStream* strm);  // Used for indexing independent elements for random access

	// EXIP specific
	void (*initHeader)(EXIStream* strm);
	errorCode (*initStream)(EXIStream* strm, BinaryBuffer buffer, EXIPSchema* schema);
	errorCode (*closeEXIStream)(EXIStream* strm);
	errorCode (*flushEXIData)(EXIStream* strm);
};

typedef struct EXISerializer EXISerializer;

/**
 * Used during serialization for easy access to the
 * EXIP serialization API */
extern const EXISerializer serialize;

/**** START: Serializer API declarations  ****/

// For handling the meta-data (document structure)

/**
 * @brief Indicates the start of the EXI body serialization
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 */
errorCode startDocument(EXIStream* strm);

/**
 * @brief Indicates the end of the EXI body serialization
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 */
errorCode endDocument(EXIStream* strm);

/**
 * @brief Encodes start of an element with qualified name qname
 *
 * @param[in, out] strm EXI stream object
 * @param[in] qname qualified name of the element
 * @param[out] valueType In case of a simple type element - the EXI type class
 * of the element. It should be used to determine which function for data handling
 * to be used for its content. If complex type - VALUE_TYPE_NONE_CLASS
 * @return Error handling code
 */
errorCode startElement(EXIStream* strm, QName qname, EXITypeClass* valueType);

/**
 * @brief Encodes the end of the last element
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 */
errorCode endElement(EXIStream* strm);



/**
 * @brief Encodes a start of an attribute with qualified name qname
 * Note that the attributes within an element must be encoded lexicographically sorted
 * when in schema mode. Sorting is done first by local name and then by namespace.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] qname qualified name of the attribute
 * @param[in] isSchemaType for schema mode, define if the value of the
 * attribute is conforming to the type defined in the schema for that attribute. If in schemaless it should be TRUE;
 * @param[out] valueType In case of a schema mode and isSchemaType == TRUE - the EXI type class
 * of the attribute. It should be used to determine which function for data handling
 * to be used for its content. Otherwise - VALUE_TYPE_NONE_CLASS
 * @return Error handling code
 * @todo Consider handling the lexicographical sorting of attributes by exip encoding utilities?
 */
errorCode attribute(EXIStream* strm, QName qname, boolean isSchemaType, EXITypeClass* valueType);

// For handling the data

/**
 * @brief Encodes integer data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] int_val value to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode intData(EXIStream* strm, Integer int_val);

/**
 * @brief Encodes boolean data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] bool_val value to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode booleanData(EXIStream* strm, boolean bool_val);

/**
 * @brief Encodes string data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] str_val value to be encoded
 * @return Error handling code
 */
errorCode stringData(EXIStream* strm, const String str_val);

/**
 * @brief Encodes float data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] float_val value to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode floatData(EXIStream* strm, Float float_val);

/**
 * @brief Encodes binary data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] binary_val value to be encoded
 * @param[in] nbytes number of bytes in binary_val
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode binaryData(EXIStream* strm, const char* binary_val, Index nbytes);

/**
 * @brief Encodes dateTime data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] dt_val value to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode dateTimeData(EXIStream* strm, EXIPDateTime dt_val);

/**
 * @brief Encodes decimal data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] dec_val value to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode decimalData(EXIStream* strm, Decimal dec_val);

/**
 * @brief Encodes list data for element or attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] itemCount the number of list items to be encoded
 * @return Error handling code
 * @note Use in schema mode only!
 */
errorCode listData(EXIStream* strm, unsigned int itemCount);

/**
 * @brief This function is only used to encode the value of xsi:type attribute
 *
 * @param[in, out] strm EXI stream object
 * @param[in] qname the qname to be encoded
 * @return Error handling code
 */
errorCode qnameData(EXIStream* strm, QName qname);

// Miscellaneous

/**
 * @brief Encode a processing instruction
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 * @todo define the parameters!
 */
errorCode processingInstruction(EXIStream* strm, const String name, const String text);

/**
 * @brief Encode a comment
 * 
 * @param strm EXI stream object
 * @param comment the comment to be encoded
 * @return errorCode 
 */
errorCode comment(EXIStream* strm, const String comment);

/**
 * @brief Encode a namespace declaration when Preserve.prefixes == TRUE
 *
 * @param[in, out] strm EXI stream object
 * @param[in] ns the namespace to be encoded
 * @param[in] prefix the prefix of the namespace to be encoded
 * @param[in] isLocalElementNS TRUE if this is the namespace locally defined for the element
 * that contains it (see local-element-ns flag in the EXI spec)
 * @return Error handling code
 */
errorCode namespaceDeclaration(EXIStream* strm, const String ns, const String prefix, boolean isLocalElementNS);

/**
 * @brief Encode a DOCTYPE
 * 
 * @param strm EXI stream object
 * @param public The public "DTD_name"
 * @param system The system/"DTD_location"
 * @param text Doctype text content as a String
 * @return errorCode 
 */
errorCode docType(EXIStream* strm, boolean public, const String system, const String text);

/**
 * @brief Encode a entity reference
 * 
 * @param strm EXI stream object
 * @param reference reference to be encoded
 * @return errorCode 
 */
errorCode entityReference(EXIStream* strm, const String reference);

// EXI specific

/**
 * @brief Encode a self Contained event used for indexing independent elements for random access
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 * @todo define the parameters!
 */
errorCode selfContained(EXIStream* strm);

// EXIP specific

/**
 * @brief Initialize the header of an EXI stream object
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 */
void initHeader(EXIStream* strm);

/**
 * @brief Initialize EXI stream object
 *
 * @param[in, out] strm EXI stream
 * @param[in, out] buffer output buffer for storing the encoded EXI stream
 * @param[in] schema a compiled schema information to be used for schema enabled processing, NULL if no schema is available
 * @return Error handling code
 */
errorCode initStream(EXIStream* strm, BinaryBuffer buffer, EXIPSchema *schema);

/**
 * @brief Destroy an EXI stream object releasing all the allocated memory for it
 *
 * @param[in, out] strm EXI stream object
 * @return Error handling code
 */
errorCode closeEXIStream(EXIStream* strm);

/**
 * @brief In case the EXI buffer (strm->buffer) is filled this function can be used to
 * flush it to some external buffer when strm->buffer.ioStrm.readWriteToStream is not available.
 * This is useful when streaming EXI data, for example, which needs to be implemented
 * without a blocking call to the flushing interface i.e. strm->buffer.ioStrm.readWriteToStream.
 *
 * @warning Padding bits to fill a byte when in bit-packed mode
 * should not be used as they will be interpreted as if being part
 * of the EXI stream. This function always flushes the EXI buffer
 * to the last byte boundary thus making sure the padding is not needed.
 *
 * @remark The proper use of this function is as follows:
 * When building the EXI body, before each call to serialize.*() functions
 * the context of the EXI stream needs to be saved to a vairable e.g.,
 * StreamContext savedContext = parser->strm.context;
 * if the serialize.*() returns EXIP_BUFFER_END_REACHED the state needs to
 * be restored with: parser->strm.context = savedContext;
 * Then the flushEXIData() function must be called to flush the
 * buffer after which the failed serialize.*() call needs to be repeated.
 *
 * @param[in, out] strm EXI stream object
 * @param[out] outBuf the next EXI stream chunk to be parsed
 * @param[in] bufSize the size in bytes of the inBuf
 * @param[out] bytesFlush bytes written to the outBuf
 * @return Error handling code
 */
errorCode flushEXIData(EXIStream* strm, char* outBuf, unsigned int bufSize, unsigned int* bytesFlush);

/****  END: Serializer API implementation  ****/


/**** START: Fast, low level API for schema encoding only ****/

/**
 * To be used by code generation tools such as static XML bindings
 * and when efficiency is of high importance
 *
 * @param[in, out] strm EXI stream
 * @param[in] ec EXI event code of the production
 * @param[in] qname used only for SE(*), AT(*), SE(uri:*), AT(uri:*) and when
 * a new prefix should be serialized in SE(QName) and AT(QName); NULL otherwise
 * @return Error handling code
 */
errorCode serializeEvent(EXIStream* strm, EventCode ec, QName* qname);

/****  END: Fast, low level API for schema encoding only ****/

#endif /* EXISERIALIZER_H_ */
