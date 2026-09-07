#include <stdlib.h>
#include "f_array.h"

FoxyArray* f_array_new_typed(size_t initial_capacity, FoxyValueType array_type) {
    FoxyArray *array = (FoxyArray*)malloc(sizeof(FoxyArray));
    if (!array) return NULL;

    array->capacity = initial_capacity > 0 ? initial_capacity : 8;
    array->count = 0;
    array->array_type = array_type;
    array->items = (FoxyValue*)calloc(array->capacity, sizeof(FoxyValue));

    if (!array->items) {
        free(array);
        return NULL;
    }

    return array;
}

// Wrapper para arreglos heterogéneos genericos
FoxyArray* f_array_new(size_t initial_capacity) {
    return f_array_new_typed(initial_capacity, FOXY_VAL_ANY);
}

void f_array_free(FoxyArray *array) {
    if (!array) return;

    if (array->items) {
        // Liberación profunda para evitar fugas de sub-elementos (strings, sub-arrays, etc.)
        for (size_t i = 0; i < array->count; i++) {
            f_value_free_contents(&array->items[i]);
        }
        free(array->items);
        array->items = NULL;
    }

    free(array);
}

bool f_array_push(FoxyArray *array, FoxyValue value) {
    if (!array) return false;

    // 1. Enforzar restricción de tipo si no es FOXY_VAL_ANY
    if (array->array_type != FOXY_VAL_ANY && value.type != array->array_type) {
        return false; // Type mismatch
    }

    // 2. Crecimiento dinámico de la capacidad
    if (array->count >= array->capacity) {
        size_t new_cap = array->capacity * 2;
        FoxyValue *new_items = (FoxyValue*)realloc(array->items, sizeof(FoxyValue) * new_cap);
        if (!new_items) return false;

        array->items = new_items;
        array->capacity = new_cap;
    }

    array->items[array->count++] = value;
    return true;
}

bool f_array_pop(FoxyArray *array, FoxyValue *out_value) {
    if (!array || array->count == 0) return false;

    array->count--;
    if (out_value) {
        *out_value = array->items[array->count];
    } else {
        // Si no se recupera el valor extraído, liberar sus recursos internos
        f_value_free_contents(&array->items[array->count]);
    }

    // Limpiar el slot liberado dentro del buffer contiguo
    array->items[array->count] = (FoxyValue){0};

    return true;
}