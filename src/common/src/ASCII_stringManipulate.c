/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file ASCII_stringManipulate.c
 * @brief String manipulation functions used for UCS <-> ASCII transformations
 *
 * @date Sep 3, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id: ASCII_stringManipulate.c 328 2013-10-30 16:00:10Z kjussakov $
 */

#include "stringManipulate.h"
#include "memManagement.h"

#define PARSING_STRING_MAX_LENGTH 100

errorCode allocateStringMemory(CharType** str, Index UCSchars)
{
	*str = EXIP_CALLOC(1, sizeof(CharType)*UCSchars);
	if((*str) == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;
	return EXIP_OK;
}

errorCode newStringWithCapacity(String* str, Index capacity)
{
	if (!isStringEmpty(str)) {
		clearString(str);
	}
	str->str = calloc(capacity, sizeof(CharType));
	if(str->str == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;
	else 
	{
		str->length = capacity;
		return EXIP_OK;
	}
}

errorCode allocateStringMemoryManaged(CharType** str, Index UCSchars, AllocList* memList)
{
	(*str) = (CharType*) memManagedAllocate(memList, sizeof(CharType)*UCSchars);
	if((*str) == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;
	return EXIP_OK;
}

/**
 * Simple translation working only for ASCII characters
 */
errorCode writeCharToString(String* str, uint32_t code_point, Index* writerPosition)
{
	if(*writerPosition >= str->length)
		return EXIP_OUT_OF_BOUND_BUFFER;
	str->str[*writerPosition] = (CharType) code_point;
	*writerPosition += 1;
	return EXIP_OK;
}

void getEmptyString(String* emptyStr)
{
	emptyStr->length = 0;
	emptyStr->str = NULL;
}

boolean isStringEmpty(const String* str)
{
	if(str == NULL || str->length == 0)
		return 1;
	return 0;
}

errorCode clearString(String* str)
{
	if (!str) {
		return EXIP_INVALID_INPUT;
	}
	if (str->length > 0) {
		free(str->str);
	}
	str->str = NULL;
	str->length = 0;
	return EXIP_OK;
}

errorCode asciiToString(const char* inStr, String* outStr, boolean clone) {
	size_t inStrLen;

	if (!inStr) {
		return EXIP_INVALID_INPUT;
	} else {
		inStrLen = strlen(inStr);
	}

	if (!outStr) {
		return EXIP_INVALID_INPUT;
	}

	if(inStrLen > 0)  // If == 0 -> empty string
	{
		if (!clone) {
			clearString(outStr);
			outStr->str = (CharType*) inStr;
			outStr->length = inStrLen;
			return EXIP_OK;
		}
		else {
			if (outStr->length < inStrLen) 
			{
				clearString(outStr);
				outStr->str = calloc(inStrLen, sizeof(CharType));
				if(outStr->str == NULL)
					return EXIP_MEMORY_ALLOCATION_ERROR;
				outStr->length = inStrLen;
			}
			memcpy(outStr->str, inStr, inStrLen);
			return EXIP_OK;
		}
	}
	else
		clearString(outStr);
	return EXIP_OK;
}

errorCode asciiToStringManaged(const char* inStr, String* outStr, AllocList* memList, boolean clone)
{
	outStr->length = strlen(inStr);
	if(outStr->length > 0)  // If == 0 -> empty string
	{
		if(clone == FALSE)
		{
			outStr->str = (CharType*) inStr;
			return EXIP_OK;
		}
		else
		{
			outStr->str = (CharType*) memManagedAllocate(memList, sizeof(CharType)*(outStr->length));
			if(outStr->str == NULL)
				return EXIP_MEMORY_ALLOCATION_ERROR;
			memcpy(outStr->str, inStr, outStr->length);
			return EXIP_OK;
		}
	}
	else
		outStr->str = NULL;
	return EXIP_OK;
}

boolean stringEqual(const String str1, const String str2)
{
	if(str1.length != str2.length)
		return 0;
	else if(str1.length == 0)
		return 1;
	else // The strings have the same length
	{
		Index i;
		for(i = 0; i < str1.length; i++)
		{
			if(str1.str[i] != str2.str[i])
				return 0;
		}
		return 1;
	}
}

boolean stringEqualToAscii(const String str1, const char* str2)
{
	if(str1.length != strlen(str2))
		return 0;
	else // The strings have the same length
	{
		Index i;
		for(i = 0; i < str1.length; i++)
		{
			if(str1.str[i] != str2[i])
				return 0;
		}
		return 1;
	}
}

int stringCompare(const String str1, const String str2)
{
	/* Check for NULL string pointers */
	if(str1.str == NULL)
	{
		if(str2.str == NULL)
			return 0;
		return -1;
	}
	else if(str2.str == NULL)
		return 1;
	else // None of the strings is NULL
	{
		int diff;
		Index i;
		for(i = 0; i < str1.length && i < str2.length; i++)
		{
			diff = str1.str[i] - str2.str[i];
			if(diff)
				return diff; // There is a difference in the characters at index i
		}
		/* Up to index i the strings have the same characters and might differ only in length*/
		return str1.length - str2.length;
	}
}

uint32_t readCharFromString(const String* str, Index* readerPosition)
{
	assert(*readerPosition < str->length);
	*readerPosition += 1;
	return (uint32_t) str->str[*readerPosition - 1];
}

errorCode cloneString(const String* src, String* newStr)
{
	if(newStr == NULL)
		return EXIP_NULL_POINTER_REF;
	newStr->str = EXIP_CALLOC(1, sizeof(CharType)*src->length);
	if(newStr->str == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;
	newStr->length = src->length;
	memcpy(newStr->str, src->str, src->length);
	return EXIP_OK;
}

errorCode cloneStringManaged(const String* src, String* newStr, AllocList* memList)
{
	if(newStr == NULL)
		return EXIP_NULL_POINTER_REF;
	newStr->str = memManagedAllocate(memList, sizeof(CharType)*src->length);
	if(newStr->str == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;
	newStr->length = src->length;
	memcpy(newStr->str, src->str, src->length);
	return EXIP_OK;
}

Index getIndexOfChar(const String* src, CharType sCh)
{
	Index i;

	for(i = 0; i < src->length; i++)
	{
		if(src->str[i] == sCh)
			return i;
	}
	return INDEX_MAX;
}

errorCode stringToInteger(const String* src, int* number)
{
	char buff[PARSING_STRING_MAX_LENGTH];
	long result;
	char *endPointer;

	if(src->length == 0 || src->length >= PARSING_STRING_MAX_LENGTH)
		return EXIP_INVALID_STRING_OPERATION;

	memcpy(buff, src->str, src->length);
	buff[src->length] = '\0';

	result = strtol(buff, &endPointer, 10);

	if(result == LONG_MAX || result == LONG_MIN || *src->str == *endPointer)
		return EXIP_INVALID_STRING_OPERATION;

	if(result >= INT_MAX || result <= INT_MIN)
		return EXIP_OUT_OF_BOUND_BUFFER;

	*number = (int) result;

	return EXIP_OK;
}

errorCode stringToInt64(const String* src, int64_t* number)
{
	char buff[PARSING_STRING_MAX_LENGTH];
	long long result;
	char *endPointer;

	if(src->length == 0 || src->length >= PARSING_STRING_MAX_LENGTH)
		return EXIP_INVALID_STRING_OPERATION;

	memcpy(buff, src->str, src->length);
	buff[src->length] = '\0';

	result = EXIP_STRTOLL(buff, &endPointer, 10);

	if(result == LLONG_MAX || result == LLONG_MIN || *src->str == *endPointer)
		return EXIP_INVALID_STRING_OPERATION;

	if(result >= LLONG_MAX || result <= LLONG_MIN)
		return EXIP_OUT_OF_BOUND_BUFFER;

	*number = (int64_t) result;

	return EXIP_OK;
}

#if EXIP_IMPLICIT_DATA_TYPE_CONVERSION

errorCode integerToString(Integer number, String* outStr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode booleanToString(boolean b, String* outStr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode floatToString(Float f, String* outStr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode decimalToString(Decimal d, String* outStr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

errorCode dateTimeToString(EXIPDateTime dt, String* outStr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
}

#endif /* EXIP_IMPLICIT_DATA_TYPE_CONVERSION */

#if EXIP_DEBUG == ON

void printString(const String* inStr)
{
	if(inStr->length == 0)
		return;

	DEBUG_OUTPUT(("%.*s", (int)inStr->length, inStr->str));
}

#endif /* EXIP_DEBUG */

void sPrintString(char * buf, const String* inStr)
{
	if(inStr->length == 0)
		return;

	sprintf(buf, "%.*s", (int)inStr->length, inStr->str);
}
