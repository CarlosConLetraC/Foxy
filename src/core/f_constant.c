#include <stdlib.h>
#include <string.h>
#include "f_constant.h"

void f_constant_pool_init(FoxyConstantPool *pool) {
    if (!pool) return;
    pool->values = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

size_t f_constant_pool_add(FoxyConstantPool *pool, FoxyValue value) {
    if (!pool) return 0;

    // Redimensionar si se alcanza la capacidad máxima actual
    if (pool->count + 1 > pool->capacity) {
        size_t old_capacity = pool->capacity;
        pool->capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        
        FoxyValue *new_values = realloc(pool->values, sizeof(FoxyValue) * pool->capacity);
        if (!new_values)
            // Manejo básico de fallo de asignación si fuera necesario
            return 0;
        pool->values = new_values;
    }

    size_t index = pool->count++;
    pool->values[index] = value;
    return index;
}

void f_constant_pool_free(FoxyConstantPool *pool) {
    if (!pool) return;

    // Liberar recursos asignados en el heap para cada constante del pool
    for (size_t i = 0; i < pool->count; i++) {
        // Las cadenas de texto se almacenan bajo el tipo de arreglo u objeto dinámico
        if ((pool->values[i].type == FOXY_VAL_ARRAY || pool->values[i].type == FOXY_VAL_OBJECT) 
            && pool->values[i].as.obj) {
            free(pool->values[i].as.obj);
            pool->values[i].as.obj = NULL;
        }
        // Nota: Extender aquí si se agregan otros tipos dinámicos en el heap en el futuro.
    }

    free(pool->values);
    f_constant_pool_init(pool);
}