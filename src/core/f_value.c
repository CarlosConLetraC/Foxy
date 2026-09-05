#include "f_value.h"
#include <stdlib.h>
#include <string.h>

// Generación del arreglo de cadenas alineado con los índices del enum mediante X-Macro
const char * const FOXY_VALUE_TYPE_STRINGS[] = {
    #define X(type_enum, type_str) [type_enum] = type_str,
    FOXY_VALUE_TYPE_LIST(X)
    #undef X
};

const char* f_value_type_to_char_array(FoxyValueType type) {
    if ((unsigned int)type >= FOXY_VAL_COUNT) return "unknown";
    return FOXY_VALUE_TYPE_STRINGS[type];
}

FoxyValue f_value_create_char_array(const char *str, size_t len) {
    FoxyValue val;
    val.type = FOXY_VAL_ARRAY;

    FoxyArray *arr = (FoxyArray*)malloc(sizeof(FoxyArray));
    if (!arr) {
        val.as.array = NULL;
        return val;
    }

    // Reservar len + 1 para asegurar el terminador nulo de C
    char *buffer = (char*)malloc(len + 1);
    if (!buffer) {
        free(arr);
        val.as.array = NULL;
        return val;
    }

    if (str && len > 0) {
        memcpy(buffer, str, len);
    }
    buffer[len] = '\0'; // Garantiza C-string válido en memoria contigua

    arr->element_type_id = FOXY_VAL_CHAR;
    arr->length = len;
    arr->data = buffer;

    val.as.array = arr;
    return val;
}

const char* f_value_get_char_array_data(const FoxyValue *val) {
    if (!val || val->type != FOXY_VAL_ARRAY || !val->as.array) return NULL;
    FoxyArray *arr = val->as.array;
    if (arr->element_type_id == FOXY_VAL_CHAR) {
        return (const char*)arr->data;
    }
    return NULL;
}

void f_value_free_contents(FoxyValue *val) {
    if (!val) return;

    switch (val->type) {
        case FOXY_VAL_ARRAY:
            if (val->as.array) {
                if (val->as.array->data) {
                    free(val->as.array->data);
                    val->as.array->data = NULL;
                }
                free(val->as.array);
                val->as.array = NULL;
            }
            break;

        case FOXY_VAL_DICT:
            if (val->as.dict) {
                free(val->as.dict);
                val->as.dict = NULL;
            }
            break;

        case FOXY_VAL_OBJECT:
        case FOXY_VAL_STRUCT:
        case FOXY_VAL_CLASS:
            if (val->as.obj) {
                free(val->as.obj);
                val->as.obj = NULL;
            }
            break;

        case FOXY_VAL_FUNCTION:
            if (val->as.func) {
                // Si la función fue asignada dinámicamente:
                free(val->as.func);
                val->as.func = NULL;
            }
            break;

        default:
            // Tipos primitivos y escalares (int, float, bool, null)
            break;
    }
}

bool f_value_is_numeric(const FoxyValue *val) {
    if (!val) return false;
    switch (val->type) {
        case FOXY_VAL_INT:
        case FOXY_VAL_NUMBER:
        case FOXY_VAL_FLOAT:
        case FOXY_VAL_DOUBLE:
        case FOXY_VAL_LONG:
        case FOXY_VAL_LONG_LONG:
        case FOXY_VAL_UNSIGNED_LONG_LONG:
        case FOXY_VAL_CHAR:
            return true;
        default:
            return false;
    }
}

bool f_value_is_pure_integer(const FoxyValue *val) {
    if (!val) return false;
    return (val->type == FOXY_VAL_INT || 
            val->type == FOXY_VAL_LONG || 
            val->type == FOXY_VAL_LONG_LONG || 
            val->type == FOXY_VAL_UNSIGNED_LONG_LONG || 
            val->type == FOXY_VAL_CHAR);
}

double f_value_as_double(const FoxyValue *val) {
    if (!val) return 0.0;
    switch (val->type) {
        case FOXY_VAL_INT:
        case FOXY_VAL_LONG:
        case FOXY_VAL_LONG_LONG:
            return (double)val->as.ival;
        case FOXY_VAL_UNSIGNED_LONG_LONG:
            return (double)((uint64_t)val->as.ival);
        case FOXY_VAL_FLOAT:
        case FOXY_VAL_DOUBLE:
        case FOXY_VAL_NUMBER:
            return val->as.dval;
        case FOXY_VAL_CHAR:
            return (double)val->as.cval;
        default:
            return 0.0;
    }
}

#define FOXY_VALUE_CMP_LIST(F) \
    F(FOXY_VAL_NULL,                true) \
    F(FOXY_VAL_VOID,                true) \
    F(FOXY_VAL_BOOL,                a->as.bval == b->as.bval) \
    F(FOXY_VAL_CHAR,                a->as.cval == b->as.cval) \
    F(FOXY_VAL_INT,                 a->as.ival == b->as.ival) \
    F(FOXY_VAL_NUMBER,              a->as.fval == b->as.fval) \
    F(FOXY_VAL_FLOAT,               a->as.fval == b->as.fval) \
    F(FOXY_VAL_DOUBLE,              a->as.dval == b->as.dval) \
    F(FOXY_VAL_LONG,                a->as.lval == b->as.lval) \
    F(FOXY_VAL_LONG_LONG,           a->as.ival == b->as.ival) \
    F(FOXY_VAL_UNSIGNED_LONG_LONG,  a->as.ival == b->as.ival) \
    F(FOXY_VAL_ARRAY,               a->as.array == b->as.array) \
    F(FOXY_VAL_DICT,                a->as.dict == b->as.dict) \
    F(FOXY_VAL_OBJECT,              a->as.obj == b->as.obj) \
    F(FOXY_VAL_STRUCT,              a->as.ptr == b->as.ptr) \
    F(FOXY_VAL_CLASS,               a->as.klass == b->as.klass) \
    F(FOXY_VAL_FUNCTION,            a->as.func == b->as.func)

bool f_value_equals(const FoxyValue *a, const FoxyValue *b) {
    if (!a || !b) return false;
    if (a->type != b->type) return false;

    switch (a->type) {
    #define EXPAND_CMP_CASE(val_type, cmp_expr) \
        case val_type: return (cmp_expr);
        
        FOXY_VALUE_CMP_LIST(EXPAND_CMP_CASE)
        
    #undef EXPAND_CMP_CASE
        default:
            return a->as.ptr == b->as.ptr;
    }
}