#include "f_value.h"
#include <stdlib.h>
#include <string.h>

// Generación del arreglo de cadenas alineado con los índices del enum mediante X-Macro
const char * const FOXY_VALUE_TYPE_STRINGS[] = {
    #define X(type_enum, type_str) [type_enum] = type_str,
    FOXY_VALUE_TYPE_LIST(X)
    #undef X
};

const char* f_value_type_to_string(FoxyValueType type) {
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
            if (val->as.function) {
                // Si la función fue asignada dinámicamente:
                free(val->as.function);
                val->as.function = NULL;
            }
            break;

        default:
            // Tipos primitivos y escalares (int, float, bool, nil)
            break;
    }
}