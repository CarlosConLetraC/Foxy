#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include "f_value.h"
#include "f_status.h"
#include "f_runtime.h"

#if FOXY_COMPILER_SUPPORTS_XMACROS
const char * const FOXY_VALUE_TYPE_NAMES[] = {
    #define F(type_enum, type_str) type_str,
    FOXY_VALUE_TYPE_LIST(F)
    #undef F
};
#else
const char * const FOXY_VALUE_TYPE_NAMES[] = {
    "null", "bool", "char", "uchar", "short",
    "ushort", "int", "uint", "long", "ulong",
    "llong", "ullong", "float", "double",
    "ldouble", "number", "array", "dict",
    "object", "struct", "class", "function",
    "enum"
};
#endif

FoxyValue f_value_new_number(double val, FoxyValueType subtype) {
    FoxyValue v;
    v.type = FOXY_VAL_NUMBER;

#if USE_COMPUTED_GOTO

    /* Tabla de dispatch basada en etiquetas para Direct Threaded Code */
    static const void* const dispatch_table[] = {
        [FOXY_VAL_CHAR]               = &&F_CHAR,
        [FOXY_VAL_UCHAR]              = &&F_UCHAR,
        [FOXY_VAL_SHORT]              = &&F_SHORT,
        [FOXY_VAL_USHORT]             = &&F_USHORT,
        [FOXY_VAL_INT]                = &&F_INT,
        [FOXY_VAL_UINT]               = &&F_UINT,
        [FOXY_VAL_LONG]               = &&F_LONG,
        [FOXY_VAL_ULONG]              = &&F_ULONG,
        [FOXY_VAL_LLONG]              = &&F_LLONG,
        [FOXY_VAL_ULLONG]             = &&F_ULLONG,
        [FOXY_VAL_FLOAT]              = &&F_FLOAT,
        [FOXY_VAL_DOUBLE]             = &&F_DOUBLE,
        [FOXY_VAL_LDOUBLE]            = &&F_LDOUBLE
    };

    /* Validación de rangos para evitar un salto de puntero fuera de límites */
    if (subtype < FOXY_VAL_CHAR || subtype > FOXY_VAL_LONG_DOUBLE || !dispatch_table[subtype])
        goto F_DEFAULT;

    /* Salto directo sin la sobrecarga de un branch table tradicional */
    goto *dispatch_table[subtype];

    F_CHAR:    v.like.f_char    = (signed char)val;        return v;
    F_UCHAR:   v.like.f_uchar   = (unsigned char)val;      return v;
    F_SHORT:   v.like.f_short   = (signed short)val;       return v;
    F_USHORT:  v.like.f_ushort  = (unsigned short)val;     return v;
    F_INT:     v.like.f_int     = (signed int)val;         return v;
    F_UINT:    v.like.f_uint    = (unsigned int)val;       return v;
    F_LONG:    v.like.f_long    = (signed long)val;        return v;
    F_ULONG:   v.like.f_ulong   = (unsigned long)val;      return v;
    F_LLONG:   v.like.f_llong   = (signed long long)val;   return v;
    F_ULLONG:  v.like.f_ullong  = (unsigned long long)val; return v;
    F_FLOAT:   v.like.f_float   = (float)val;              return v;
    F_DOUBLE:  v.like.f_double  = (double)val;             return v;
    F_LDOUBLE: v.like.f_ldouble = (long double)val;        return v;
    F_DEFAULT:
        f_runtime_error(
            NULL,
            FOXY_STATUS_ERROR_TYPE_MISMATCH,
            "Subtipo numérico inválido '%s' (%d) en f_value_new_number()",
            f_value_type_to_string(subtype),
            subtype
        );
        exit(FOXY_STATUS_ERROR_TYPE_MISMATCH);

#else

    /* Fallback portable con switch-case (C99 Estándar) */
    switch (subtype) {
        case FOXY_VAL_CHAR:               v.like.f_char = (signed char)val;          break;
        case FOXY_VAL_UCHAR:              v.like.f_uchar = (unsigned char)val;       break;
        case FOXY_VAL_SHORT:              v.like.f_short = (signed short)val;        break;
        case FOXY_VAL_USHORT:             v.like.f_ushort = (unsigned short)val;     break;
        case FOXY_VAL_INT:                v.like.f_int = (signed int)val;            break;
        case FOXY_VAL_UINT:               v.like.f_uint = (unsigned int)val;         break;
        case FOXY_VAL_LONG:               v.like.f_long = (signed long)val;          break;
        case FOXY_VAL_ULONG:              v.like.f_ulong = (unsigned long)val;       break;
        case FOXY_VAL_LLONG:              v.like.f_llong = (signed long long)val;    break;
        case FOXY_VAL_ULLONG:             v.like.f_ullong = (unsigned long long)val; break;
        case FOXY_VAL_FLOAT:              v.like.f_float = (float)val;               break;
        case FOXY_VAL_DOUBLE:             v.like.f_double = (double)val;             break;
        case FOXY_VAL_LDOUBLE:            v.like.f_ldouble = (long double)val;       break;
        default:
            f_runtime_error(
                NULL,
                FOXY_STATUS_ERROR_TYPE_MISMATCH,
                "Subtipo numérico inválido '%s' (%d) en f_value_new_number()",
                f_value_type_to_string(subtype),
                subtype
            );
            exit(FOXY_STATUS_ERROR_TYPE_MISMATCH);
    }
    return v;
#endif
}

// const char *f_value_type_to_string(FoxyValueType type) {
//     if (type >= 0 && type < FOXY_VAL_COUNT) {
//         return FOXY_VALUE_TYPE_NAMES[type];
//     }
//     return "desconocido";
// }