#include "f_constant.h"
#include "f_value.h"
#include <stdlib.h>

void f_constant_pool_init(FoxyConstantPool *pool) {
    if (!pool) return;
    pool->values = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

// Función auxiliar interna para liberar el contenido dinámico de un FoxyValue
static void f_value_free_contents(FoxyValue *val) {
    if (!val) return;

    switch (val->type) {
        case FOXY_VAL_ARRAY:
            // 1. Si el valor contiene un FoxyArray estructurado en el Heap
            if (val->as.array) {
                if (val->as.array->data) {
                    free(val->as.array->data);
                    val->as.array->data = NULL;
                }
                free(val->as.array);
                val->as.array = NULL;
            }
            // 2. Si se utilizó el campo string/sval como bloque raw char* independiente
            if (val->as.string) {
                free(val->as.string);
                val->as.string = NULL;
            }
            break;

        case FOXY_VAL_DICT:
            // Si el diccionario posee memoria dinámica propia, invocar su liberador aquí
            if (val->as.dict) {
                // f_dict_free(val->as.dict);
                val->as.dict = NULL;
            }
            break;

        case FOXY_VAL_OBJECT:
        case FOXY_VAL_STRUCT:
        case FOXY_VAL_CLASS:
            // Liberación de estructuras basadas en objetos del Heap
            if (val->as.object) {
                free(val->as.object);
                val->as.object = NULL;
            }
            break;

        case FOXY_VAL_FUNCTION:
            // Si la función está embebida o referenciada directamente como valor
            if (val->as.native_fn) {
                // Limpieza específica si aplica
                val->as.native_fn = NULL;
            }
            break;

        default:
            // Tipos primitivos puros (null, char, int, number, float, double, long, bool)
            // No requieren operaciones de liberación en el Heap.
            break;
    }
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