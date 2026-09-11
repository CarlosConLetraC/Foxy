#ifndef F_VALUE_H
#define F_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations. . . */
typedef struct FoxyArray FoxyArray;
typedef struct FoxyDict FoxyDict;
typedef struct FoxyObject FoxyObject;
typedef struct FoxyStruct FoxyStruct;
typedef struct FoxyClass FoxyClass;
typedef struct FoxyFunction FoxyFunction;
typedef struct FoxyEnum FoxyEnum;

#define FOXY_VALUE_TYPE_LIST(F) \
    F(FOXY_VAL_NULL,                "null")     /*[00]*/\
    F(FOXY_VAL_BOOL,                "bool")     /*[01]*/\
    F(FOXY_VAL_CHAR,                "char")     /*[02]*/\
    F(FOXY_VAL_UCHAR,               "uchar")    /*[03]*/\
    F(FOXY_VAL_SHORT,               "short")    /*[04]*/\
    F(FOXY_VAL_USHORT,              "ushort")   /*[05]*/\
    F(FOXY_VAL_INT,                 "int")      /*[06]*/\
    F(FOXY_VAL_UINT,                "uint")     /*[07]*/\
    F(FOXY_VAL_LONG,                "long")     /*[08]*/\
    F(FOXY_VAL_ULONG,               "ulong")    /*[09]*/\
    F(FOXY_VAL_LLONG,               "llong")    /*[10]*/\
    F(FOXY_VAL_ULLONG,              "ullong")   /*[11]*/\
    F(FOXY_VAL_FLOAT,               "float")    /*[12]*/\
    F(FOXY_VAL_DOUBLE,              "double")   /*[13]*/\
    F(FOXY_VAL_LDOUBLE,             "ldouble")  /*[14]*/\
    F(FOXY_VAL_NUMBER,              "number")   /*[15] Metatipo numérico dinámico */\
    F(FOXY_VAL_ARRAY,               "array")    /*[16]*/\
    F(FOXY_VAL_DICT,                "dict")     /*[17]*/\
    F(FOXY_VAL_OBJECT,              "object")   /*[18]*/\
    F(FOXY_VAL_STRUCT,              "struct")   /*[19]*/\
    F(FOXY_VAL_CLASS,               "class")    /*[20]*/\
    F(FOXY_VAL_FUNCTION,            "function") /*[21]*/\
    F(FOXY_VAL_ENUM,                "enum")     /*[22]*/

typedef enum {
    #define F(type_enum, type_str) type_enum,
    FOXY_VALUE_TYPE_LIST(F)
    #undef F
    FOXY_VAL_COUNT
} FoxyValueType;

typedef struct FoxyValue {
    FoxyValueType type; // Etiqueta única del tipo de valor 
    union {
        bool f_bool;
        signed char f_char;
        unsigned char f_uchar;
        signed short f_short;
        unsigned short f_ushort;
        signed int f_int;
        unsigned int f_uint;
        signed long f_long;
        unsigned long f_ulong;
        signed long long f_llong;
        unsigned long long f_ullong;
        float f_float;
        double f_double;
        long double f_ldouble;

        /* Punteros a tipos complejos en Heap */
        FoxyArray *f_array;
        FoxyDict *f_dict;
        FoxyObject *f_object;
        FoxyStruct *f_struct;
        FoxyClass *f_class;
        FoxyFunction *f_function;
        FoxyEnum *f_enum;
    } like;
} FoxyValue;

extern const char * const FOXY_VALUE_TYPE_NAMES[];

/* Valores constantes globales */
#define FOXY_CONSTANT_VALUE_NULL        ((FoxyValue){ .type = FOXY_VAL_NULL })
#define FOXY_CONSTANT_VALUE_BOOL_TRUE   ((FoxyValue){ .type = FOXY_VAL_BOOL, .like.f_bool = true })
#define FOXY_CONSTANT_VALUE_BOOL_FALSE  ((FoxyValue){ .type = FOXY_VAL_BOOL, .like.f_bool = false })

/* Constructores de prototipos rápidos (Macros / Compound Literals C99) */
#define f_value_new_char(v)         ((FoxyValue){ .type = FOXY_VAL_CHAR, .like.f_char = (v) })
#define f_value_new_uchar(v)        ((FoxyValue){ .type = FOXY_VAL_UCHAR, .like.f_uchar = (v) })
#define f_value_new_short(v)        ((FoxyValue){ .type = FOXY_VAL_SHORT, .like.f_short = (v) })
#define f_value_new_ushort(v)       ((FoxyValue){ .type = FOXY_VAL_USHORT, .like.f_ushort = (v) })
#define f_value_new_int(v)          ((FoxyValue){ .type = FOXY_VAL_INT, .like.f_int = (v) })
#define f_value_new_uint(v)         ((FoxyValue){ .type = FOXY_VAL_UINT, .like.f_uint = (v) })
#define f_value_new_long(v)         ((FoxyValue){ .type = FOXY_VAL_LONG, .like.f_long = (v) })
#define f_value_new_ulong(v)        ((FoxyValue){ .type = FOXY_VAL_ULONG, .like.f_ulong = (v) })
#define f_value_new_llong(v)        ((FoxyValue){ .type = FOXY_VAL_LLONG, .like.f_llong = (v) })
#define f_value_new_ullong(v)       ((FoxyValue){ .type = FOXY_VAL_ULLONG, .like.f_ullong = (v) })
#define f_value_new_float(v)        ((FoxyValue){ .type = FOXY_VAL_FLOAT, .like.f_float = (v) })
#define f_value_new_double(v)       ((FoxyValue){ .type = FOXY_VAL_DOUBLE, .like.f_double = (v) })
#define f_value_new_ldouble(v)      ((FoxyValue){ .type = FOXY_VAL_LDOUBLE, .like.f_ldouble = (v) })
FoxyValue f_value_new_number(double val, FoxyValueType subtype);
#define f_value_new_array(v)        ((FoxyValue){ .type = FOXY_VAL_ARRAY, .like.f_array = (v) })
#define f_value_new_dict(v)         ((FoxyValue){ .type = FOXY_VAL_DICT, .like.f_dict = (v) })
#define f_value_new_object(v)       ((FoxyValue){ .type = FOXY_VAL_OBJECT, .like.f_object = (v) })
#define f_value_new_struct(v)       ((FoxyValue){ .type = FOXY_VAL_STRUCT, .like.f_struct = (v) })
#define f_value_new_class(v)        ((FoxyValue){ .type = FOXY_VAL_CLASS, .like.f_class = (v) })
#define f_value_new_function(v)     ((FoxyValue){ .type = FOXY_VAL_FUNCTION, .like.f_function = (v) })
#define f_value_new_enum(v)         ((FoxyValue){ .type = FOXY_VAL_ENUM, .like.f_enum = (v) })

/* Checks y utilidades rápidas */
#define f_value_is_numeric(v)       ((v).type >= (FOXY_VAL_BOOL) && (v).type <= (FOXY_VAL_NUMBER))
#define f_value_type_to_string(t)   (((t) >= 0 && (t) < (FOXY_VAL_COUNT)) ? FOXY_VALUE_TYPE_NAMES[(t)] : "unknown")

#endif // F_VALUE_H