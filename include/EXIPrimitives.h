#ifndef EXIPRIMITIVES_H
#define EXIPRIMITIVES_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef bool boolean;
#define TRUE true
#define FALSE false

#ifndef EXIP_UNSIGNED_INTEGER
# define EXIP_UNSIGNED_INTEGER uint64_t
#endif

typedef EXIP_UNSIGNED_INTEGER UnsignedInteger;

#ifndef EXIP_INTEGER
# define EXIP_INTEGER int64_t
#endif

typedef EXIP_INTEGER Integer;

/**
 * Represents base 10 (decimal) floating-point data.
 * The default Float representation in EXIP.
 * Maps directly to the EXI Float datatype.
 * Used for float and decimal data.
 *
 * @see http://www.w3.org/TR/2011/REC-exi-20110310/#encodingFloat
 */
struct EXIFloat
{
	int64_t mantissa;
	int16_t exponent;
};

#ifndef EXIP_FLOAT
# define EXIP_FLOAT struct EXIFloat
#endif

typedef EXIP_FLOAT Float;

/**
 * Used for the content handler interface for decimal values.
 * Application which require support for different type of decimal
 * representation (IEEE 754 or ISO/IEC/IEEE 60559:2011 standards) can
 * override this macro and re-define the decimal encoding/decoding
 * functions (not recommended). Instead:
 * On platforms supporting decimal floating types the conversion
 * between EXIP_FLOAT and _Decimal64 or _Decimal128 should be done
 * in the application code.
 *
 * @see http://gcc.gnu.org/onlinedocs/gcc/Decimal-Float.html#Decimal-Float
 * @see http://speleotrove.com/decimal/
 */
#ifndef EXIP_DECIMAL
# define EXIP_DECIMAL Float
#endif

typedef EXIP_DECIMAL Decimal;

#ifndef EXIP_INDEX
# define EXIP_INDEX size_t
#endif

typedef EXIP_INDEX Index;

#ifndef EXIP_INDEX_MAX
# define EXIP_INDEX_MAX SIZE_MAX
#endif

#define INDEX_MAX EXIP_INDEX_MAX

#ifndef EXIP_SMALL_INDEX
# define EXIP_SMALL_INDEX size_t
#endif

typedef EXIP_SMALL_INDEX SmallIndex;

#ifndef EXIP_SMALL_INDEX_MAX
# define EXIP_SMALL_INDEX_MAX SIZE_MAX
#endif

#define SMALL_INDEX_MAX EXIP_SMALL_INDEX_MAX

#ifndef EXIP_IMPLICIT_DATA_TYPE_CONVERSION
# define EXIP_IMPLICIT_DATA_TYPE_CONVERSION ON
#endif

/**
 * Defines the encoding used for characters.
 * It is dependent on the implementation of the stringManipulate.h functions
 * The default is ASCII characters (ASCII_stringManipulate.c)
 */
#ifndef CHAR_TYPE
# define CHAR_TYPE char
#endif

typedef CHAR_TYPE CharType;


#ifndef EXIP_STRTOLL
/** strtoll() function */
# define EXIP_STRTOLL strtoll
#endif

/**
 * Represents the length prefixed strings in EXIP
 */
struct StringType
{
	CharType* str;
	Index length;
};

typedef struct StringType String;

#define EMPTY_STRING (String){.str = NULL, .length = 0}

#endif // EXIPRIMITIVES_H