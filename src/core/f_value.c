#include "f_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* Tabla global de cadenas de texto para cada tipo de valor */
#if FOXY_COMPILER_SUPPORTS_XMACROS
const char * const FOXY_VALUE_value_NAMES[] = {
    #define F(type_enum, type_str) type_str,
    FOXY_VALUE_value_LIST(F)
    #undef F
};
#else
const char * const FOXY_VALUE_value_NAMES[] = {
    "null", "bool", "char", "uchar", "short", "ushort",
    "int", "uint", "long", "ulong", "llong", "ullong",
    "float", "double", "ldouble", "number", "void", "array",
    "dict", "object", "struct", "class", "function", "enum"
};
#endif

FoxyValue f_value_new_number(double val, FoxyValueType subtype) {
    FoxyValue v;
    v.type = subtype;

    switch (subtype) {
        case FOXY_VAL_CHAR:    v.like.f_char = (signed char)val; break;
        case FOXY_VAL_UCHAR:   v.like.f_uchar = (unsigned char)val; break;
        case FOXY_VAL_SHORT:   v.like.f_short = (signed short)val; break;
        case FOXY_VAL_USHORT:  v.like.f_ushort = (unsigned short)val; break;
        case FOXY_VAL_INT:     v.like.f_int = (signed int)val; break;
        case FOXY_VAL_UINT:    v.like.f_uint = (unsigned int)val; break;
        case FOXY_VAL_LONG:    v.like.f_long = (signed long)val; break;
        case FOXY_VAL_ULONG:   v.like.f_ulong = (unsigned long)val; break;
        case FOXY_VAL_LLONG:   v.like.f_llong = (signed long long)val; break;
        case FOXY_VAL_ULLONG:  v.like.f_ullong = (unsigned long long)val; break;
        case FOXY_VAL_FLOAT:   v.like.f_float = (float)val; break;
        case FOXY_VAL_DOUBLE:  
        case FOXY_VAL_NUMBER:
        default:               
            v.type = FOXY_VAL_DOUBLE;
            v.like.f_double = val; 
            break;
        case FOXY_VAL_LDOUBLE: v.like.f_ldouble = (long double)val; break;
    }

    return v;
}

double f_value_to_double(FoxyValue val) {
    switch (val.type) {
        case FOXY_VAL_BOOL:    return val.like.f_bool ? 1.0 : 0.0;
        case FOXY_VAL_CHAR:    return (double)val.like.f_char;
        case FOXY_VAL_UCHAR:   return (double)val.like.f_uchar;
        case FOXY_VAL_SHORT:   return (double)val.like.f_short;
        case FOXY_VAL_USHORT:  return (double)val.like.f_ushort;
        case FOXY_VAL_INT:     return (double)val.like.f_int;
        case FOXY_VAL_UINT:    return (double)val.like.f_uint;
        case FOXY_VAL_LONG:    return (double)val.like.f_long;
        case FOXY_VAL_ULONG:   return (double)val.like.f_ulong;
        case FOXY_VAL_LLONG:   return (double)val.like.f_llong;
        case FOXY_VAL_ULLONG:  return (double)val.like.f_ullong;
        case FOXY_VAL_FLOAT:   return (double)val.like.f_float;
        case FOXY_VAL_DOUBLE:  
        case FOXY_VAL_NUMBER:  return val.like.f_double;
        case FOXY_VAL_LDOUBLE: return (double)val.like.f_ldouble;
        default:               return 0.0;
    }
}

int64_t f_value_to_int64(FoxyValue val) {
    switch (val.type) {
        case FOXY_VAL_BOOL:    return val.like.f_bool ? 1 : 0;
        case FOXY_VAL_CHAR:    return (int64_t)val.like.f_char;
        case FOXY_VAL_UCHAR:   return (int64_t)val.like.f_uchar;
        case FOXY_VAL_SHORT:   return (int64_t)val.like.f_short;
        case FOXY_VAL_USHORT:  return (int64_t)val.like.f_ushort;
        case FOXY_VAL_INT:     return (int64_t)val.like.f_int;
        case FOXY_VAL_UINT:    return (int64_t)val.like.f_uint;
        case FOXY_VAL_LONG:    return (int64_t)val.like.f_long;
        case FOXY_VAL_ULONG:   return (int64_t)val.like.f_ulong;
        case FOXY_VAL_LLONG:   return (int64_t)val.like.f_llong;
        case FOXY_VAL_ULLONG:  return (int64_t)val.like.f_ullong;
        case FOXY_VAL_FLOAT:   return (int64_t)val.like.f_float;
        case FOXY_VAL_DOUBLE:  
        case FOXY_VAL_NUMBER:  return (int64_t)val.like.f_double;
        case FOXY_VAL_LDOUBLE: return (int64_t)val.like.f_ldouble;
        default:               return 0;
    }
}

FoxyValue f_value_cast_numeric(FoxyValue val, FoxyValueType target_type) {
    if (!f_value_type_is_numeric(val.type)) return val;
    return f_value_new_number(f_value_to_double(val), target_type);
}

bool f_value_numeric_equals(FoxyValue a, FoxyValue b) {
    if (!f_value_type_is_numeric(a.type) || !f_value_type_is_numeric(b.type)) {
        return false;
    }
    return f_value_to_double(a) == f_value_to_double(b);
}

void f_value_free(FoxyValue *value) {
    if (!value) return;

    if (f_value_is_heap(*value)) {
        /* TODO: Integrar con el GC / allocador de Foxy cuando las 
           estructuras completas de Heap estén definidas */
        value->like.f_object = NULL;
    }

    value->type = FOXY_VAL_NULL;
}

bool f_value_equals(FoxyValue a, FoxyValue b) {
    if (a.type != b.type) {
        /* Si ambos son numéricos, permitir comparación por valor real */
        if (f_value_type_is_numeric(a.type) && f_value_type_is_numeric(b.type)) {
            return f_value_numeric_equals(a, b);
        }
        return false;
    }

    switch (a.type) {
        case FOXY_VAL_NULL:     return true;
        case FOXY_VAL_VOID:     return true;
        case FOXY_VAL_BOOL:     return a.like.f_bool == b.like.f_bool;
        case FOXY_VAL_CHAR:     return a.like.f_char == b.like.f_char;
        case FOXY_VAL_UCHAR:    return a.like.f_uchar == b.like.f_uchar;
        case FOXY_VAL_SHORT:    return a.like.f_short == b.like.f_short;
        case FOXY_VAL_USHORT:   return a.like.f_ushort == b.like.f_ushort;
        case FOXY_VAL_INT:      return a.like.f_int == b.like.f_int;
        case FOXY_VAL_UINT:     return a.like.f_uint == b.like.f_uint;
        case FOXY_VAL_LONG:     return a.like.f_long == b.like.f_long;
        case FOXY_VAL_ULONG:    return a.like.f_ulong == b.like.f_ulong;
        case FOXY_VAL_LLONG:    return a.like.f_llong == b.like.f_llong;
        case FOXY_VAL_ULLONG:   return a.like.f_ullong == b.like.f_ullong;
        case FOXY_VAL_FLOAT:    return a.like.f_float == b.like.f_float;
        case FOXY_VAL_DOUBLE:   
        case FOXY_VAL_NUMBER:   return a.like.f_double == b.like.f_double;
        case FOXY_VAL_LDOUBLE:  return a.like.f_ldouble == b.like.f_ldouble;
        case FOXY_VAL_ARRAY:    return a.like.f_array == b.like.f_array;
        case FOXY_VAL_DICT:     return a.like.f_dict == b.like.f_dict;
        case FOXY_VAL_OBJECT:   return a.like.f_object == b.like.f_object;
        case FOXY_VAL_STRUCT:   return a.like.f_struct == b.like.f_struct;
        case FOXY_VAL_CLASS:    return a.like.f_class == b.like.f_class;
        case FOXY_VAL_FUNCTION: return a.like.f_function == b.like.f_function;
        case FOXY_VAL_ENUM:     return false;
        default:                return false;
    }
}

void f_value_print(FoxyValue value) {
    switch (value.type) {
        case FOXY_VAL_NULL:     printf("null"); break;
        case FOXY_VAL_VOID:     printf("void"); break;
        case FOXY_VAL_BOOL:     printf("%s", value.like.f_bool ? "true" : "false"); break;
        case FOXY_VAL_CHAR:     printf("%d", value.like.f_char); break;
        case FOXY_VAL_UCHAR:    printf("%u", value.like.f_uchar); break;
        case FOXY_VAL_SHORT:    printf("%d", value.like.f_short); break;
        case FOXY_VAL_USHORT:   printf("%u", value.like.f_ushort); break;
        case FOXY_VAL_INT:      printf("%d", value.like.f_int); break;
        case FOXY_VAL_UINT:     printf("%u", value.like.f_uint); break;
        case FOXY_VAL_LONG:     printf("%ld", value.like.f_long); break;
        case FOXY_VAL_ULONG:    printf("%lu", value.like.f_ulong); break;
        case FOXY_VAL_LLONG:    printf("%" PRId64, (int64_t)value.like.f_llong); break;
        case FOXY_VAL_ULLONG:   printf("%" PRIu64, (uint64_t)value.like.f_ullong); break;
        case FOXY_VAL_FLOAT:    printf("%g", (double)value.like.f_float); break;
        case FOXY_VAL_DOUBLE:   
        case FOXY_VAL_NUMBER:   printf("%g", value.like.f_double); break;
        case FOXY_VAL_LDOUBLE:  printf("%Lg", value.like.f_ldouble); break;
        case FOXY_VAL_ARRAY:    printf("<array %p>", (void*)value.like.f_array); break;
        case FOXY_VAL_DICT:     printf("<dict %p>", (void*)value.like.f_dict); break;
        case FOXY_VAL_OBJECT:   printf("<object %p>", (void*)value.like.f_object); break;
        case FOXY_VAL_STRUCT:   printf("<struct %p>", (void*)value.like.f_struct); break;
        case FOXY_VAL_CLASS:    printf("<class %p>", (void*)value.like.f_class); break;
        case FOXY_VAL_FUNCTION: printf("<function %p>", (void*)value.like.f_function); break;
        case FOXY_VAL_ENUM:     printf("<enum>"); break;
        default:                printf("<unknown>"); break;
    }
}

bool f_value_is_ancestor_of(FoxyValue parent, FoxyValue child) {
    if (parent.type != FOXY_VAL_CLASS && parent.type != FOXY_VAL_OBJECT) return false;
    if (child.type != FOXY_VAL_CLASS && child.type != FOXY_VAL_OBJECT) return false;

    /* TODO: Traversal de la cadena de herencia de FoxyClass */
    return false;
}

bool f_value_is_descendant_of(FoxyValue child, FoxyValue parent) {
    return f_value_is_ancestor_of(parent, child);
}