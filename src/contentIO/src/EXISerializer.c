/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file EXISerializer.c
 * @brief Implementation of the serializer of EXI streams
 *
 * @date Sep 30, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "EXISerializer.h"
#include "grammars.h"
#include "memManagement.h"
#include "sTables.h"
#include "headerEncode.h"
#include "bodyEncode.h"
#include "hashtable.h"
#include "stringManipulate.h"
#include "streamEncode.h"
#include "initSchemaInstance.h"
#include "ioUtil.h"
#include "streamEncode.h"

#define ASCII_0 0x30
#define ASCII_9 0x39
#define ASCII_DOT 0x2e
/**
 * The handler to be used by the applications to serialize EXI streams
 */
const EXISerializer serialize ={startDocument,
								endDocument,
								startElement,
								endElement,
								attribute,
								intData,
								booleanData,
								stringData,
								floatData,
								binaryData,
								dateTimeData,
								decimalData,
								listData,
								qnameData,
								processingInstruction,
								comment,
								namespaceDeclaration,
								docType,
								entityReference,
								encodeHeader, /*exiHeader*/
								selfContained,
								initHeader,
								initStream,
								closeEXIStream};

#if EXI_PROFILE_DEFAULT

extern const String XML_SCHEMA_INSTANCE;
extern const String URI_2_LN[];

static errorCode encodeATXsiType(EXIStream* strm);
static errorCode encodeAnyType(EXIStream* strm);
#endif

void initHeader(EXIStream* strm)
{
	strm->header.has_cookie = FALSE;
	strm->header.has_options = FALSE;
	strm->header.is_preview_version = FALSE;
	strm->header.version_number = 1;
	makeDefaultOpts(&strm->header.opts);
}

errorCode initStream(EXIStream* strm, BinaryBuffer buffer, EXIPSchema* schema)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">EXI stream initialization \n"));

	TRY(checkOptionValues(&strm->header.opts));

	TRY(initAllocList(&(strm->memList)));
	strm->buffer = buffer;
	strm->context.bitPointer = 0;
	strm->context.bufferIndx = 0;
	strm->context.currAttr.uriId = URI_MAX;
	strm->context.currAttr.lnId = LN_MAX;
	strm->context.expectATData = FALSE;
	strm->context.isNilType = FALSE;
	strm->context.attrTypeId = INDEX_MAX;
	strm->gStack = NULL;
	strm->valueTable.value = NULL;
	strm->valueTable.count = 0;
	strm->schema = NULL;

	if(strm->header.opts.valuePartitionCapacity > 0)
	{
		TRY(createValueTable(&strm->valueTable));
	}

	if(strm->header.opts.schemaIDMode == SCHEMA_ID_NIL)
	{
		// When the "schemaId" element in the EXI options document contains the xsi:nil attribute
		// with its value set to true, no schema information is used for processing the EXI body
		// (i.e. a schema-less EXI stream)
		strm->schema = NULL;
#if EXI_PROFILE_DEFAULT
		DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n> EXI Profile mode require schema mode processing"));
		return EXIP_INVALID_EXI_INPUT;
#endif

#if DEBUG_CONTENT_IO == ON && EXIP_DEBUG_LEVEL <= WARNING
		if(schema != NULL)
			DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n> Ignored schema information - SCHEMA_ID_NIL mode required"));
#endif
	}
	else if(strm->header.opts.schemaIDMode == SCHEMA_ID_EMPTY)
	{
		// When the value of the "schemaId" element is empty, no user defined schema information
		// is used for processing the EXI body; however, the built-in XML schema types are available for use in the EXI body
#if DEBUG_CONTENT_IO == ON && EXIP_DEBUG_LEVEL <= WARNING
		if(schema != NULL)
			DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n> Ignored out-of-band schema information. Schema mode built-in types required"));
#endif
		strm->schema = memManagedAllocate(&strm->memList, sizeof(EXIPSchema));
		if(strm->schema == NULL)
			return EXIP_MEMORY_ALLOCATION_ERROR;

		TRY(initSchema(strm->schema, INIT_SCHEMA_BUILD_IN_TYPES));

		if(WITH_FRAGMENT(strm->header.opts.enumOpt))
		{
			TRY(createFragmentGrammar(strm->schema, NULL, 0));
		}
		else
		{
			TRY(createDocGrammar(strm->schema, NULL, 0));
		}
	}
	else if(schema != NULL)
	{
		/* Schema enabled mode*/
		if(strm->header.opts.schemaIDMode == SCHEMA_ID_SET)
		{
			if(isStringEmpty(&strm->header.opts.schemaID))
				return EXIP_INVALID_EXIP_CONFIGURATION;
		}

		if(WITH_FRAGMENT(strm->header.opts.enumOpt))
		{
			/* Fragment document grammar */
			// TODO: create a Schema-informed Fragment Grammar from the EXIP schema object
			return EXIP_NOT_IMPLEMENTED_YET;
		}
		else
		{
			strm->schema = schema;
		}
	}

	if(strm->schema == NULL)
	{
		// Schema-less mode
		if(strm->header.opts.schemaIDMode == SCHEMA_ID_SET)
		{
			DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n> SCHEMA_ID_SET mode required, but NULL schema set"));
			return EXIP_INVALID_EXIP_CONFIGURATION;
		}

#if EXI_PROFILE_DEFAULT
		DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n> EXI Profile mode require schema mode processing"));
		return EXIP_INVALID_EXI_INPUT;
#endif

		strm->schema = memManagedAllocate(&strm->memList, sizeof(EXIPSchema));
		if(strm->schema == NULL)
			return EXIP_MEMORY_ALLOCATION_ERROR;

		TRY(initSchema(strm->schema, INIT_SCHEMA_SCHEMA_LESS_MODE));

		if(WITH_FRAGMENT(strm->header.opts.enumOpt))
		{
			TRY(createFragmentGrammar(strm->schema, NULL, 0));
		}
		else
		{
			TRY(createDocGrammar(strm->schema, NULL, 0));
		}
	}

	{
		QNameID emptyQNameID = {URI_MAX, LN_MAX};
		TRY(pushGrammar(&strm->gStack, emptyQNameID, &strm->schema->docGrammar));
	}
	// #DOCUMENT#
	// Hashtable for fast look-up of global values in the table.
	// Only used when:
	// serializing &&
	// valuePartitionCapacity > 50  &&   //for small table full-scan will work better
	// valueMaxLength > 0 && // this is essentially equal to valuePartitionCapacity == 0
	// HASH_TABLE_USE == ON // build configuration parameter
#if HASH_TABLE_USE
	if(strm->header.opts.valuePartitionCapacity > DEFAULT_VALUE_ENTRIES_NUMBER &&
			strm->header.opts.valueMaxLength > 0)
	{
		strm->valueTable.hashTbl = create_hashtable(INITIAL_HASH_TABLE_SIZE, djbHash, stringEqual);
		if(strm->valueTable.hashTbl == NULL)
			return EXIP_HASH_TABLE_ERROR;
	}
	else
		strm->valueTable.hashTbl = NULL;
#endif
	return EXIP_OK;
}

errorCode startDocument(EXIStream* strm)
{
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Start doc serialization\n"));

	if(strm->gStack->grammar == NULL && strm->gStack->currNonTermID != GR_DOC_CONTENT)
		return EXIP_INCONSISTENT_PROC_STATE;

	return EXIP_OK;
}

errorCode endDocument(EXIStream* strm)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">End doc serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	tmp_err_code = encodeProduction(strm, EVENT_ED_CLASS, TRUE, NULL, VALUE_TYPE_NONE_CLASS, &prodHit);

	// Store the size of the encoded stream contained in the BinaryBuffer in the BinaryBuffer.bufContent
	strm->buffer.bufContent = strm->context.bufferIndx + (strm->context.bitPointer > 0);

	return tmp_err_code;
}

errorCode startElement(EXIStream* strm, QName qname, EXITypeClass* valueType)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start element serialization\n"));

	if(strm->gStack->grammar == NULL)
#if EXI_PROFILE_DEFAULT
	{
		TRY(encodeATXsiType(strm));
		TRY(encodeAnyType(strm));
	}
#else
	{
		return EXIP_INCONSISTENT_PROC_STATE;
	}
#endif

	*valueType = VALUE_TYPE_NONE_CLASS;

	TRY(encodeProduction(strm, EVENT_SE_CLASS, TRUE, &qname, VALUE_TYPE_NONE_CLASS, &prodHit));

	if(GET_PROD_EXI_EVENT(prodHit.content) == EVENT_SE_ALL)
	{
		EXIGrammar* elemGrammar = NULL;
		QNameID tmpQid;

		TRY(encodeQName(strm, qname, EVENT_SE_ALL, &tmpQid));

		// New element grammar is pushed on the stack
#if EXI_PROFILE_DEFAULT
		if(GET_LN_URI_QNAME(strm->schema->uriTable, tmpQid).elemGrammar == EXI_PROFILE_STUB_GRAMMAR_INDX)
			elemGrammar = NULL;
		else
#endif
		elemGrammar = GET_ELEM_GRAMMAR_QNAMEID(strm->schema, tmpQid);

		if(elemGrammar != NULL) // The grammar is found
		{
			TRY(pushGrammar(&(strm->gStack), tmpQid, elemGrammar));
		}
		else
		{
#if BUILD_IN_GRAMMARS_USE
			EXIGrammar newElementGrammar;
			Index dynArrIndx;
			TRY(createBuiltInElementGrammar(&newElementGrammar, strm));

			TRY(addDynEntry(&strm->schema->grammarTable.dynArray, &newElementGrammar, &dynArrIndx));

			GET_LN_URI_QNAME(strm->schema->uriTable, tmpQid).elemGrammar = dynArrIndx;
			TRY(pushGrammar(&(strm->gStack), tmpQid, &strm->schema->grammarTable.grammar[dynArrIndx]));
#elif EXI_PROFILE_DEFAULT
			// Leave the grammar NULL - if the next event is valid AT(xsi:type)
			// then its value will be the next grammar.
			// If the next event is not valid AT(xsi:type) - then the event
			// AT(xsi:type="anyType") will be inserted beforehand
			TRY(pushGrammar(&(strm->gStack), tmpQid, elemGrammar));

			return EXIP_OK;
#else
			DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, (">Build-in element grammars are not supported by this configuration \n"));
			assert(FALSE);
			return EXIP_INCONSISTENT_PROC_STATE;
#endif
		}
	}
	else if(GET_PROD_EXI_EVENT(prodHit.content) == EVENT_SE_QNAME)
	{
		EXIGrammar* elemGrammar = NULL;

		TRY(encodePfxQName(strm, &qname, EVENT_SE_QNAME, prodHit.qnameId.uriId));

		// New element grammar is pushed on the stack
		if(IS_BUILT_IN_ELEM(strm->gStack->grammar->props))  // If the current grammar is build-in Element grammar ...
		{
			elemGrammar = GET_ELEM_GRAMMAR_QNAMEID(strm->schema, prodHit.qnameId);
		}
		else
		{
			elemGrammar = &strm->schema->grammarTable.grammar[prodHit.typeId];
		}

		if(elemGrammar != NULL) // The grammar is found
			TRY(pushGrammar(&(strm->gStack), prodHit.qnameId, elemGrammar));
		else
			return EXIP_INCONSISTENT_PROC_STATE;  // The event require the presence of Element Grammar previously created
	}
	else
		return EXIP_NOT_IMPLEMENTED_YET;

	if(!IS_BUILT_IN_ELEM(strm->gStack->grammar->props))  // If the current grammar is not build-in Element grammar ...
	{
		GrammarRule* currentRule;
		currentRule = &strm->gStack->grammar->rule[strm->gStack->currNonTermID];
		assert(currentRule->production);
		if(GET_PROD_EXI_EVENT(currentRule->production[currentRule->pCount-1].content) == EVENT_CH)
		{
			// This must be simple type grammar
			if(currentRule->production[currentRule->pCount-1].typeId == INDEX_MAX)
				*valueType = VALUE_TYPE_UNTYPED_CLASS;
			else
				*valueType = GET_VALUE_TYPE_CLASS(GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[currentRule->production[currentRule->pCount-1].typeId].content));
		}
	}

	return EXIP_OK;
}

errorCode endElement(EXIStream* strm)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">End element serialization\n"));

	if(strm->gStack->grammar == NULL)
#if EXI_PROFILE_DEFAULT
	{
		TRY(encodeATXsiType(strm));
		TRY(encodeAnyType(strm));
	}
#else
	{
		return EXIP_INCONSISTENT_PROC_STATE;
	}
#endif

	TRY(encodeProduction(strm, EVENT_EE_CLASS, TRUE, NULL, VALUE_TYPE_NONE_CLASS, &prodHit));

	if(strm->gStack->currNonTermID == GR_VOID_NON_TERMINAL)
		popGrammar(&(strm->gStack));
	else
		return EXIP_INCONSISTENT_PROC_STATE;

	return EXIP_OK;
}

errorCode attribute(EXIStream* strm, QName qname, boolean isSchemaType, EXITypeClass* valueType)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start attribute serialization\n"));

	assert(qname.uri != NULL);
	assert(qname.localName != NULL);

	if(strm->gStack->grammar == NULL)
#if EXI_PROFILE_DEFAULT
	{
		// if qname == xsi:type
		if(stringEqual(*qname.uri, XML_SCHEMA_INSTANCE) &&
				stringEqual(*qname.localName, URI_2_LN[XML_SCHEMA_INSTANCE_TYPE_ID]))
		{
			// Encode the xsi:type and wait for a QName type serialization;
			// Leave the current grammar NULL
			TRY(encodeATXsiType(strm));
			strm->context.expectATData = TRUE;
			strm->context.currAttr.uriId = XML_SCHEMA_INSTANCE_ID;
			strm->context.currAttr.lnId = XML_SCHEMA_INSTANCE_TYPE_ID;
			strm->context.attrTypeId = SIMPLE_TYPE_QNAME;
			*valueType = VALUE_TYPE_QNAME_CLASS;
			return EXIP_OK;
		}
		else
		{
			// Insert first an AT(xsi:type="anyType) event
			TRY(encodeATXsiType(strm));
			TRY(encodeAnyType(strm));
		}
	}
#else
	{
		return EXIP_INCONSISTENT_PROC_STATE;
	}
#endif

	// Check for trying to represent namespace declarations with AT event
	// Only in debug mode!
	// See EXI errata about namespace declarations - http://www.w3.org/XML/EXI/exi-10-errata
#if EXIP_DEBUG == ON && EXIP_DEBUG_LEVEL == WARNING
	{
		String ln = *qname.localName;

		if(!stringEqualToAscii(*qname.uri, "http://www.w3.org/2000/xmlns/"))
		{
			DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>Trying to represent namespace declarations with AT event\n"));
			return EXIP_INVALID_EXI_INPUT;
		}

		if(ln.length != 0)
		{
			ln.length = 5;
			if(!stringEqualToAscii(ln, "xmlns"))
			{
				DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>Trying to represent namespace declarations with AT event\n"));
				return EXIP_INVALID_EXI_INPUT;
			}
		}
	}
#endif

	TRY(encodeProduction(strm, EVENT_AT_CLASS, isSchemaType, &qname, VALUE_TYPE_NONE_CLASS, &prodHit));

	if(prodHit.typeId == INDEX_MAX)
		*valueType = VALUE_TYPE_NONE_CLASS;
	else
		*valueType = GET_VALUE_TYPE_CLASS(GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[prodHit.typeId].content));

	if(GET_PROD_EXI_EVENT(prodHit.content) == EVENT_AT_ALL)
	{
		TRY(encodeQName(strm, qname, EVENT_AT_ALL, &strm->context.currAttr));

		if(IS_SCHEMA(strm->gStack->grammar->props) && strm->context.currAttr.uriId == XML_SCHEMA_INSTANCE_ID &&
				(strm->context.currAttr.uriId == XML_SCHEMA_INSTANCE_TYPE_ID || strm->context.currAttr.uriId == XML_SCHEMA_INSTANCE_NIL_ID))
		{
			DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, (">In schema-informed grammars, xsi:type and xsi:nil attributes MUST NOT be represented using AT(*) terminal\n"));
			return EXIP_INCONSISTENT_PROC_STATE;
		}
	}
	else if(GET_PROD_EXI_EVENT(prodHit.content) == EVENT_AT_QNAME)
	{
		strm->context.currAttr.uriId = prodHit.qnameId.uriId;
		strm->context.currAttr.lnId = prodHit.qnameId.lnId;

		TRY(encodePfxQName(strm, &qname, EVENT_AT_QNAME, prodHit.qnameId.uriId));
	}
	else
		return EXIP_NOT_IMPLEMENTED_YET;

	strm->context.expectATData = TRUE;
	strm->context.attrTypeId = prodHit.typeId;

	return EXIP_OK;
}

errorCode intData(EXIStream* strm, Integer int_val)
{
	Index intTypeId;
	QNameID qnameID;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start integer data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute or list item
	{
		intTypeId = strm->context.attrTypeId;
		qnameID = strm->context.currAttr;
		strm->context.expectATData -= 1;
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		// TODO: passing the type class is not enough:
		// we need to check if the int value (int_val) fits in the
		// production value content description.
		// If it does not fit we need to again use untyped second level production
		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_INTEGER_CLASS, &prodHit));
		qnameID = strm->gStack->currQNameID;
		intTypeId = prodHit.typeId;
	}

	return encodeIntData(strm, int_val, qnameID, intTypeId);
}

errorCode booleanData(EXIStream* strm, boolean bool_val)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	boolean isXsiNilAttr = FALSE;
	Index booleanTypeId;
	EXIType exiType;
	QNameID qnameID;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start boolean data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		if(strm->context.currAttr.uriId == XML_SCHEMA_INSTANCE_ID && strm->context.currAttr.lnId == XML_SCHEMA_INSTANCE_NIL_ID)
		{
			// xsi:nill
			isXsiNilAttr = TRUE;
		}
		booleanTypeId = strm->context.attrTypeId;
		qnameID = strm->context.currAttr;
	}
	else
	{
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_BOOLEAN_CLASS, &prodHit));
		booleanTypeId = prodHit.typeId;
		qnameID = strm->gStack->currQNameID;
	}

	if(booleanTypeId != INDEX_MAX)
		exiType = GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[booleanTypeId].content);
	else
		exiType = VALUE_TYPE_NONE;

	if(exiType == VALUE_TYPE_BOOLEAN)
	{
		TRY(encodeBoolean(strm, bool_val));
	}
	else if(exiType == VALUE_TYPE_STRING || exiType == VALUE_TYPE_UNTYPED || exiType == VALUE_TYPE_NONE)
	{
		//       1) Print Warning
		//       2) convert the boolean to sting
		//       3) encode string
		String tmpStr;

		DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>Boolean to String conversion required \n"));
#if EXIP_IMPLICIT_DATA_TYPE_CONVERSION
		TRY(booleanToString(bool_val, &tmpStr));
		TRY(encodeStringData(strm, tmpStr, qnameID, booleanTypeId));
		EXIP_MFREE(tmpStr.str);
#else
		return EXIP_INVALID_EXI_INPUT;
#endif
	}
	else
	{
	    DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Production type is not a boolean\n"));
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	if(IS_SCHEMA(strm->gStack->grammar->props) && isXsiNilAttr && bool_val)
	{
		// In a schema-informed grammar && xsi:nil == TRUE
		strm->context.isNilType = TRUE;
		strm->gStack->currNonTermID = GR_START_TAG_CONTENT;
	}

	return EXIP_OK;
}

errorCode stringData(EXIStream* strm, const String str_val)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	QNameID qnameID;
	Index typeId;
	EXIType exiType;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start string data serialization\n"));

	if(strm->gStack->grammar == NULL)
#if EXI_PROFILE_DEFAULT
	{
		TRY(encodeATXsiType(strm));
		TRY(encodeAnyType(strm));
	}
#else
	{
		return EXIP_INCONSISTENT_PROC_STATE;
	}
#endif

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		qnameID = strm->context.currAttr;
		typeId = strm->context.attrTypeId;
	}
	else
	{
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_STRING_CLASS, &prodHit));

		qnameID = strm->gStack->currQNameID;
		typeId = prodHit.typeId;
	}

	if(typeId != INDEX_MAX)
		exiType = GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[typeId].content);
	else
		exiType = VALUE_TYPE_NONE;

	if(exiType == VALUE_TYPE_STRING || exiType == VALUE_TYPE_UNTYPED || exiType == VALUE_TYPE_NONE)
	{
		return encodeStringData(strm, str_val, qnameID, typeId);
	}
	else
	{
		DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Expected typed data, but string provided\n"));
		return EXIP_INVALID_EXI_INPUT;
	}
}

errorCode floatData(EXIStream* strm, Float float_val)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Index typeId;
	QNameID qnameID;
	EXIType exiType;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start float data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		typeId = strm->context.attrTypeId;
		qnameID = strm->context.currAttr;
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_FLOAT_CLASS, &prodHit));
		qnameID = strm->gStack->currQNameID;
		typeId = prodHit.typeId;
	}

	if(typeId != INDEX_MAX)
		exiType = GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[typeId].content);
	else
		exiType = VALUE_TYPE_NONE;

	if(exiType == VALUE_TYPE_FLOAT)
	{
		return encodeFloatValue(strm, float_val);
	}
	else if(exiType == VALUE_TYPE_STRING || exiType == VALUE_TYPE_UNTYPED || exiType == VALUE_TYPE_NONE)
	{
		//       1) Print Warning
		//       2) convert the float to sting
		//       3) encode string
		String tmpStr;

		DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>Float to String conversion required \n"));
#if EXIP_IMPLICIT_DATA_TYPE_CONVERSION
		TRY(floatToString(float_val, &tmpStr));
		TRY(encodeStringData(strm, tmpStr, qnameID, typeId));
		EXIP_MFREE(tmpStr.str);
#else
		return EXIP_INVALID_EXI_INPUT;
#endif
	}
	else
	{
	    DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Production type is not a float\n"));
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	return EXIP_OK;
}

errorCode binaryData(EXIStream* strm, const char* binary_val, Index nbytes)
{
	Index typeId;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start binary data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		typeId = strm->context.attrTypeId;
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_BINARY_CLASS, &prodHit));
		typeId = prodHit.typeId;
	}

	if(typeId == INDEX_MAX || GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[typeId].content) != VALUE_TYPE_BINARY)
	{
		// Binary to string conversion is not supported
	    DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Production type is not a binary\n"));
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	return encodeBinary(strm, (char *)binary_val, nbytes);
}

errorCode dateTimeData(EXIStream* strm, EXIPDateTime dt_val)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Index typeId;
	QNameID qnameID;
	EXIType exiType;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start dateTime data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		typeId = strm->context.attrTypeId;
		qnameID = strm->context.currAttr;
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		// TODO: passing the type class is not enough:
		// we need to check if the dt_val value fits in the
		// production value content description.
		// If it does not fit we need to again use untyped second level production
		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_DATE_TIME_CLASS, &prodHit));
		typeId = prodHit.typeId;
		qnameID = strm->gStack->currQNameID;
	}

	if(typeId != INDEX_MAX)
		exiType = GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[typeId].content);
	else
		exiType = VALUE_TYPE_NONE;

	if(GET_EVENT_CLASS(exiType) == VALUE_TYPE_DATE_TIME_CLASS)
	{
		return encodeDateTimeValue(strm, exiType, dt_val);
	}
	else if(exiType == VALUE_TYPE_STRING || exiType == VALUE_TYPE_UNTYPED || exiType == VALUE_TYPE_NONE)
	{
		//       1) Print Warning
		//       2) convert the dateTime to sting
		//       3) encode string
		String tmpStr;

		DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>DateTime to String conversion required \n"));
#if EXIP_IMPLICIT_DATA_TYPE_CONVERSION
		TRY(dateTimeToString(dt_val, &tmpStr));
		TRY(encodeStringData(strm, tmpStr, qnameID, typeId));
		EXIP_MFREE(tmpStr.str);
#else
		return EXIP_INVALID_EXI_INPUT;
#endif
	}
	else
	{
	    DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Production type is not a dateTime\n"));
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	return EXIP_OK;
}

errorCode decimalData(EXIStream* strm, Decimal dec_val)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	Index typeId;
	QNameID qnameID;
	EXIType exiType;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start decimal data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		typeId = strm->context.attrTypeId;
		qnameID = strm->context.currAttr;
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_DECIMAL_CLASS, &prodHit));
		qnameID = strm->gStack->currQNameID;
		typeId = prodHit.typeId;
	}

	if(typeId != INDEX_MAX)
		exiType = GET_EXI_TYPE(strm->schema->simpleTypeTable.sType[typeId].content);
	else
		exiType = VALUE_TYPE_NONE;

	if(exiType == VALUE_TYPE_DECIMAL)
	{
		return encodeDecimalValue(strm, dec_val);
	}
	else if(exiType == VALUE_TYPE_STRING || exiType == VALUE_TYPE_UNTYPED || exiType == VALUE_TYPE_NONE)
	{
		//       1) Print Warning
		//       2) convert the float to sting
		//       3) encode string
		String tmpStr;

		DEBUG_MSG(WARNING, DEBUG_CONTENT_IO, ("\n>Decimal to String conversion required \n"));
#if EXIP_IMPLICIT_DATA_TYPE_CONVERSION
		TRY(decimalToString(dec_val, &tmpStr));
		TRY(encodeStringData(strm, tmpStr, qnameID, typeId));
		EXIP_MFREE(tmpStr.str);
#else
		return EXIP_INVALID_EXI_INPUT;
#endif
	}
	else
	{
	    DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, ("\n>Production type is not a decimal\n"));
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	return EXIP_OK;
}

errorCode listData(EXIStream* strm, unsigned int itemCount)
{
	Index typeId;
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start list data serialization\n"));

	if(strm->gStack->grammar == NULL)
		return EXIP_INCONSISTENT_PROC_STATE;

	if(strm->context.expectATData > 0) // Value for an attribute
	{
		strm->context.expectATData -= 1;
		typeId = strm->context.attrTypeId;

		// TODO: is it allowed to have list with elements lists??? To be checked...
	}
	else
	{
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
		Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

		TRY(encodeProduction(strm, EVENT_CH_CLASS, TRUE, NULL, VALUE_TYPE_LIST_CLASS, &prodHit));
	}

	strm->context.expectATData = itemCount;
 	strm->context.attrTypeId = strm->schema->simpleTypeTable.sType[typeId].length; // The actual type of the list items

	return encodeUnsignedInteger(strm, (UnsignedInteger) itemCount);
}

errorCode qnameData(EXIStream* strm, QName qname)
{
	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, ("\n>Start qname data serialization\n"));
	// Only allowed for AT(xsi:type) productions
	// TODO: Add the case when Preserve.lexicalValues option value is true - instead of Qname encode it as String

	if(strm->context.expectATData > 0 && strm->context.currAttr.uriId == XML_SCHEMA_INSTANCE_ID && strm->context.currAttr.lnId == XML_SCHEMA_INSTANCE_TYPE_ID)
	{
		// Value for the attribute xsi:type
		QNameID qnameId;
		EXIGrammar* newGrammar = NULL;
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

		strm->context.expectATData -= 1;

		TRY(encodeQName(strm, qname, EVENT_AT_ALL, &qnameId));

		// New type grammar is pushed on the stack if it exists
		newGrammar = GET_TYPE_GRAMMAR_QNAMEID(strm->schema, qnameId);

		if(newGrammar != NULL)
		{
			// The grammar is found
			// preserve the currQNameID
			QNameID currQNameID = strm->gStack->currQNameID;
			popGrammar(&(strm->gStack));
			TRY(pushGrammar(&(strm->gStack), currQNameID, newGrammar));
		}
		else if(strm->gStack->grammar == NULL)
			return EXIP_INCONSISTENT_PROC_STATE;

		return EXIP_OK;
	}
	else
		return EXIP_INCONSISTENT_PROC_STATE;
}

errorCode processingInstruction(EXIStream* strm, const String name, const String text)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode comment(EXIStream* strm, const String comment)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode namespaceDeclaration(EXIStream* strm, const String ns, const String prefix, boolean isLocalElementNS)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	SmallIndex uriId;
	Production prodHit = {0, INDEX_MAX, {URI_MAX, LN_MAX}};

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Start namespace declaration\n"));

	if(!IS_PRESERVED(strm->header.opts.preserve, PRESERVE_PREFIXES))
		return EXIP_INVALID_EXI_INPUT;

	if(strm->gStack->grammar == NULL)
#if EXI_PROFILE_DEFAULT
	{
		EventCode tmpEvCode;

		tmpEvCode.length = 2;
		if(GET_LN_URI_QNAME(strm->schema->uriTable, strm->gStack->currQNameID).elemGrammar == EXI_PROFILE_STUB_GRAMMAR_INDX)
		{
			// This is the case when there is a top level AT(xsi:type) production inserted and hence
			// the first part of the event code with value 1 must be encoded before the second level part of the event code
			tmpEvCode.part[0] = 1;
			tmpEvCode.bits[0] = 1;
		}
		else
		{
			tmpEvCode.part[0] = 0;
			tmpEvCode.bits[0] = 0;
		}

		tmpEvCode.part[1] = 2;
		tmpEvCode.bits[1] = 2 + (IS_PRESERVED(strm->header.opts.preserve, PRESERVE_PREFIXES) ||
				WITH_SELF_CONTAINED(strm->header.opts.enumOpt) || IS_PRESERVED(strm->header.opts.preserve, PRESERVE_DTD)
				|| IS_PRESERVED(strm->header.opts.preserve, PRESERVE_COMMENTS) || IS_PRESERVED(strm->header.opts.preserve, PRESERVE_PIS));
		// serialize  NS event code
		TRY(writeEventCode(strm, tmpEvCode));
		// serialize  NS event content
		TRY(encodeUri(strm, (String*) &ns, &uriId));
		if(strm->schema->uriTable.uri[uriId].pfxTable == NULL)
		{
			TRY(createPfxTable(&strm->schema->uriTable.uri[uriId].pfxTable));
		}
		TRY(encodePfx(strm, uriId, (String*) &prefix));
		// Leave the current grammar NULL
		return encodeBoolean(strm, isLocalElementNS);
	}
#else
	{
		return EXIP_INCONSISTENT_PROC_STATE;
	}
#endif

	TRY(encodeProduction(strm, EVENT_NS_CLASS, FALSE, NULL, VALUE_TYPE_NONE_CLASS, &prodHit));
	TRY(encodeUri(strm, (String*) &ns, &uriId));

	if(strm->schema->uriTable.uri[uriId].pfxTable == NULL)
	{
		TRY(createPfxTable(&strm->schema->uriTable.uri[uriId].pfxTable));
	}
	TRY(encodePfx(strm, uriId, (String*) &prefix));

	return encodeBoolean(strm, isLocalElementNS);
}

errorCode docType(EXIStream* strm, boolean public, const String system, const String text)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode entityReference(EXIStream* strm, const String reference)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode selfContained(EXIStream* strm)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode closeEXIStream(EXIStream* strm)
{
	errorCode tmp_err_code = EXIP_OK;

	while(strm->gStack != NULL)
	{
		popGrammar(&strm->gStack);
	}

	// Flush the buffer first if there is an output Stream
	if(strm->buffer.ioStrm.readWriteToStream != NULL || strm->buffer.bufStrm.buf != NULL)
	{
		Index numBytesWritten = 0;
		writeToStream(&(strm->buffer), 0, strm->context.bufferIndx + 1, &numBytesWritten);
		if(numBytesWritten < strm->context.bufferIndx + 1) {
			tmp_err_code = EXIP_BUFFER_END_REACHED;
		}
	}

	freeAllMem(strm);
	return tmp_err_code;
}

errorCode flushEXIData(EXIStream* strm, char* outBuf, unsigned int bufSize, unsigned int* bytesFlush)
{
	char leftOverBits;

	if(bufSize < strm->context.bufferIndx)
		return EXIP_OUT_OF_BOUND_BUFFER;

	leftOverBits = strm->buffer.buf[strm->context.bufferIndx];

	memcpy(outBuf, strm->buffer.buf, strm->context.bufferIndx);

	strm->buffer.buf[0] = leftOverBits;
	strm->context.bufferIndx = 0;

	*bytesFlush = strm->context.bufferIndx;

	return EXIP_OK;
}

errorCode serializeEvent(EXIStream* strm, EventCode ec, QName* qname)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	GrammarRule* currentRule;
	Production* tmpProd = NULL;

	if(strm->gStack->currNonTermID >=  strm->gStack->grammar->count)
		return EXIP_INCONSISTENT_PROC_STATE;

#if BUILD_IN_GRAMMARS_USE
	if(IS_BUILT_IN_ELEM(strm->gStack->grammar->props))  // If the current grammar is build-in Element grammar ...
		currentRule = (GrammarRule*) &((DynGrammarRule*) strm->gStack->grammar->rule)[strm->gStack->currNonTermID];
	else
#endif
		currentRule = &strm->gStack->grammar->rule[strm->gStack->currNonTermID];

	TRY(writeEventCode(strm, ec));

	if(ec.length == 1)
	{
		tmpProd = &currentRule->production[currentRule->pCount - 1 - ec.part[0]];
	}
	else if(ec.length == 2)
		return EXIP_NOT_IMPLEMENTED_YET;
	else // length == 3
		return EXIP_NOT_IMPLEMENTED_YET;

	strm->gStack->currNonTermID = GET_PROD_NON_TERM(tmpProd->content);

	switch(GET_PROD_EXI_EVENT(tmpProd->content))
	{
		case EVENT_SD:
			return EXIP_OK;
		break;
		case EVENT_ED:
			return EXIP_OK;
		break;
		case EVENT_AT_QNAME:

			strm->context.currAttr.uriId = tmpProd->qnameId.uriId;
			strm->context.currAttr.lnId = tmpProd->qnameId.lnId;

			TRY(encodePfxQName(strm, qname, EVENT_AT_QNAME, tmpProd->qnameId.uriId));

			strm->context.expectATData = TRUE;
			strm->context.attrTypeId = tmpProd->typeId;

		break;
		case EVENT_AT_URI:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_AT_ALL:
			if(qname == NULL)
				return EXIP_NULL_POINTER_REF;

			TRY(encodeQName(strm, *qname, EVENT_AT_ALL, &strm->context.currAttr));

			strm->context.expectATData = TRUE;
			strm->context.attrTypeId = tmpProd->typeId;
		break;
		case EVENT_SE_QNAME:
		{
			EXIGrammar* elemGrammar = NULL;

			TRY(encodePfxQName(strm, qname, EVENT_SE_QNAME, tmpProd->qnameId.uriId));

			// New element grammar is pushed on the stack
			if(IS_BUILT_IN_ELEM(strm->gStack->grammar->props))  // If the current grammar is build-in Element grammar ...
			{
				elemGrammar = GET_ELEM_GRAMMAR_QNAMEID(strm->schema, strm->gStack->currQNameID);
			}
			else
			{
				elemGrammar = &strm->schema->grammarTable.grammar[tmpProd->typeId];
			}

			if(elemGrammar != NULL) // The grammar is found
				TRY(pushGrammar(&(strm->gStack), tmpProd->qnameId, elemGrammar));
			else
				return EXIP_INCONSISTENT_PROC_STATE;  // The event require the presence of Element Grammar previously created
		}
		break;
		case EVENT_SE_URI:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_SE_ALL:
		{
			EXIGrammar* elemGrammar = NULL;
			QNameID tmpQid;

			if(qname == NULL)
				return EXIP_NULL_POINTER_REF;

			TRY(encodeQName(strm, *qname, EVENT_SE_ALL, &tmpQid));

			// New element grammar is pushed on the stack
#if EXI_PROFILE_DEFAULT
			if(GET_LN_URI_QNAME(strm->schema->uriTable, tmpQid).elemGrammar == EXI_PROFILE_STUB_GRAMMAR_INDX)
				elemGrammar = NULL;
			else
#endif
			elemGrammar = GET_ELEM_GRAMMAR_QNAMEID(strm->schema, tmpQid);

			if(elemGrammar != NULL) // The grammar is found
			{
				TRY(pushGrammar(&(strm->gStack), tmpQid, elemGrammar));
			}
			else
			{
#if BUILD_IN_GRAMMARS_USE
				EXIGrammar newElementGrammar;
				Index dynArrIndx;
				TRY(createBuiltInElementGrammar(&newElementGrammar, strm));

				TRY(addDynEntry(&strm->schema->grammarTable.dynArray, &newElementGrammar, &dynArrIndx));

				GET_LN_URI_QNAME(strm->schema->uriTable, tmpQid).elemGrammar = dynArrIndx;

				TRY(pushGrammar(&(strm->gStack), tmpQid, &strm->schema->grammarTable.grammar[dynArrIndx]));
#elif EXI_PROFILE_DEFAULT
				// Leave the grammar NULL - if the next event is valid AT(xsi:type)
				// then its value will be the next grammar.
				// If the next event is not valid AT(xsi:type) - then the event
				// AT(xsi:type="anyType") will be inserted beforehand
				TRY(pushGrammar(&(strm->gStack), tmpQid, elemGrammar));

				return EXIP_OK;
#else
				DEBUG_MSG(ERROR, DEBUG_CONTENT_IO, (">Build-in element grammars are not supported by this configuration \n"));
				assert(FALSE);
				return EXIP_INCONSISTENT_PROC_STATE;
#endif
			}
		}
		break;
		case EVENT_EE:
			assert(strm->gStack->currNonTermID == GR_VOID_NON_TERMINAL);

			popGrammar(&(strm->gStack));
		break;
		case EVENT_CH:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_NS:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_CM:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_PI:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_DT:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_ER:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		case EVENT_SC:
			return EXIP_NOT_IMPLEMENTED_YET;
		break;
		default:
			return EXIP_INCONSISTENT_PROC_STATE;
	}

	return EXIP_OK;
}

#if EXI_PROFILE_DEFAULT
static errorCode encodeATXsiType(EXIStream* strm)
{
	// This indicates that there was a SE(*) which let the
	// grammar be NULL. Insert an AT(xsi:type="anyType") event
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EventCode tmpEvCode;

	tmpEvCode.length = 2;
	if(GET_LN_URI_QNAME(strm->schema->uriTable, strm->gStack->currQNameID).elemGrammar == EXI_PROFILE_STUB_GRAMMAR_INDX)
	{
		// This is the case when there is a top level AT(xsi:type) production inserted and hence
		// the first part of the event code with value 1 must be encoded before the second level part of the event code
		tmpEvCode.part[0] = 1;
		tmpEvCode.bits[0] = 1;
	}
	else
	{
		tmpEvCode.part[0] = 0;
		tmpEvCode.bits[0] = 0;
		GET_LN_URI_QNAME(strm->schema->uriTable, strm->gStack->currQNameID).elemGrammar = EXI_PROFILE_STUB_GRAMMAR_INDX;
	}

	tmpEvCode.part[1] = 1;
	tmpEvCode.bits[1] = 2 + (IS_PRESERVED(strm->header.opts.preserve, PRESERVE_PREFIXES) ||
			WITH_SELF_CONTAINED(strm->header.opts.enumOpt) || IS_PRESERVED(strm->header.opts.preserve, PRESERVE_DTD)
			|| IS_PRESERVED(strm->header.opts.preserve, PRESERVE_COMMENTS) || IS_PRESERVED(strm->header.opts.preserve, PRESERVE_PIS));
	// serialize  AT(xsi:type)
	TRY(writeEventCode(strm, tmpEvCode));
	TRY(encodeNBitUnsignedInteger(strm, getBitsNumber(strm->schema->uriTable.count), XML_SCHEMA_INSTANCE_ID + 1));
	TRY(encodeUnsignedInteger(strm, 0));
	TRY(encodeNBitUnsignedInteger(strm, getBitsNumber((unsigned int)(strm->schema->uriTable.uri[XML_SCHEMA_INSTANCE_ID].lnTable.count - 1)), XML_SCHEMA_INSTANCE_TYPE_ID));

	return EXIP_OK;
}

static errorCode encodeAnyType(EXIStream* strm)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	EXIGrammar* anyGrammar = NULL;
	QNameID anyTypeId;
	// preserve the currQNameID
	QNameID currQNameID = strm->gStack->currQNameID;

	// serialize "xs:anyType"
	TRY(encodeNBitUnsignedInteger(strm, getBitsNumber(strm->schema->uriTable.count), XML_SCHEMA_NAMESPACE_ID + 1));
	TRY(encodeUnsignedInteger(strm, 0));
	TRY(encodeNBitUnsignedInteger(strm, getBitsNumber((unsigned int)(strm->schema->uriTable.uri[XML_SCHEMA_NAMESPACE_ID].lnTable.count - 1)), SIMPLE_TYPE_ANY_TYPE));

	// "xs:anyType" grammar is pushed on the stack instead of the NULL one
	popGrammar(&(strm->gStack));
	anyTypeId.uriId = XML_SCHEMA_NAMESPACE_ID;
	anyTypeId.lnId = SIMPLE_TYPE_ANY_TYPE;
	anyGrammar = GET_TYPE_GRAMMAR_QNAMEID(strm->schema, anyTypeId);
	assert(anyGrammar != NULL);

	TRY(pushGrammar(&(strm->gStack), currQNameID, anyGrammar));

	return EXIP_OK;
}
#endif
