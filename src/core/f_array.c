#include <stdlib.h>
#include "f_value.h"
#include "f_array.h"

FoxyArray* f_array_new_typed(size_t initial_length, FoxyValueType fval) {
    FoxyArray *arr = (FoxyArray *)malloc(sizeof(FoxyArray));
    if (!arr) return NULL;

    arr->array_type = fval;
    arr->count = 0;
    arr->length = initial_length > 0 ? initial_length : 8;

    arr->items = (FoxyValue *)calloc(arr->length, sizeof(FoxyValue));
    if (!arr->items) {
        free(arr);
        return NULL;
    }

    return arr;
}

FoxyArray* f_array_new(size_t initial_length, FoxyValueType fval) {
    return f_array_new_typed(initial_length, fval);
}

void f_array_free(FoxyArray *array) {
    if (!array) return;

    if (array->items) {
        for (size_t i = 0; i < array->count; i++) f_value_free_contents(&array->items[i]);
        free(array->items);
        array->items = NULL;
    }

    free(array);
}

bool f_array_push(FoxyArray *array, FoxyValue value) {
    if (!array) return false;

    if (array->array_type != FOXY_VAL_NULL && value.type != array->array_type)
        return false;

    if (array->count >= array->length) {
        size_t new_len = array->length * 2;
        FoxyValue *new_items = (FoxyValue *)realloc(array->items, sizeof(FoxyValue) * new_len);
        if (!new_items) return false;
        array->items = new_items;
        array->length = new_len;
    }

    array->items[array->count++] = value;
    return true;
}

bool f_array_pop(FoxyArray *array, FoxyValue *out_value) {
    if (!array || array->count == 0) return false;

    array->count--;
    if (out_value)
        *out_value = array->items[array->count];
    else
        f_value_free_contents(&array->items[array->count]);

    array->items[array->count] = (FoxyValue){0};
    return true;
}