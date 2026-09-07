#ifndef FOXY_ARRAY_H
    #define FOXY_ARRAY_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"

    typedef struct FoxyArray {
        FoxyValue *items;
        size_t length;            // Capacidad máxima de memoria reservada
        size_t count;             // Número actual de elementos
        FoxyValueType array_type; // FOXY_VAL_NULL para heterogéneos
    } FoxyArray;

    FoxyArray* f_array_new(size_t initial_length, FoxyValueType fval);
    FoxyArray* f_array_new_typed(size_t initial_length, FoxyValueType fval);
    void       f_array_free(FoxyArray *array);

    bool       f_array_push(FoxyArray *array, FoxyValue value);
    bool       f_array_pop(FoxyArray *array, FoxyValue *out_value);
#endif // FOXY_ARRAY_H