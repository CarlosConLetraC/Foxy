#include "f_constant.h"
#include "f_value.h"
#include <stdlib.h>

void f_constant_pool_init(FoxyConstantPool *pool) {
    if (!pool) return;
    pool->values = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

size_t f_constant_pool_add(FoxyConstantPool *pool, FoxyValue value) {
    if (!pool) return 0;

    if (pool->count >= pool->capacity) {
        size_t new_cap = pool->capacity == 0 ? 8 : pool->capacity * 2;
        FoxyValue *new_values = (FoxyValue *)realloc(pool->values, sizeof(FoxyValue) * new_cap);
        if (!new_values) return (size_t)-1; // Error de asignación
        pool->values = new_values;
        pool->capacity = new_cap;
    }

    pool->values[pool->count] = value;
    return pool->count++;
}

void f_constant_pool_free(FoxyConstantPool *pool) {
    if (!pool) return;

    if (pool->values) {
        for (size_t i = 0; i < pool->count; i++) {
            // Liberar recursivamente los datos internos de cada constante (FoxyConstant es FoxyValue)
            f_value_free_contents(&pool->values[i]);
        }
        free(pool->values);
        pool->values = NULL;
    }
    
    pool->count = 0;
    pool->capacity = 0;
}