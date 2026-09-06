#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_function.h"
#include "f_gc.h" // Importar el GC

FoxyFunction* f_function_create(const char *name, uint8_t arity) {
    // Asignación controlada por el heap del GC
    FoxyFunction *func = (FoxyFunction*)f_gc_allocate(FOXY_HEAP_FUNCTION, sizeof(FoxyFunction), (void(*)(void*))f_function_free);
    if (!func) return NULL;

    func->name = name ? strdup(name) : NULL;
    func->arity = arity;
    func->type = FOXY_FUNCTION_USER; // Por defecto se crea como función de usuario. . .
    
    // Inicializar los campos internos de la unión de usuario. . .
    func->as.user.code = NULL;
    func->as.user.code_size = 0;
    func->as.user.code_capacity = 0;

    func->as.user.constants = NULL;
    func->as.user.constants_count = 0;
    func->as.user.constants_capacity = 0;

    func->as.user.locals_count = 0;
    func->as.user.locals_capacity = 0;
    func->as.user.env = NULL;

    return func;
}

// Opcional: Constructor útil si necesitas registrar funciones nativas de C fácilmente
FoxyFunction* f_function_create_native(const char *name, uint8_t arity, FoxyNativeFn native_ptr) {
    FoxyFunction *func = (FoxyFunction*)f_gc_allocate(FOXY_HEAP_FUNCTION, sizeof(FoxyFunction), (void(*)(void*))f_function_free);
    if (!func) return NULL;

    func->name = name ? strdup(name) : NULL;
    func->arity = arity;
    func->type = FOXY_FUNCTION_NATIVE;
    
    func->as.native.function_ptr = native_ptr;

    return func;
}

void f_function_free(FoxyFunction *fn) {
    if (!fn) return;

    // 1. Liberar el nombre de la función si fue asignado dinámicamente
    if (fn->name) {
        free(fn->name);
        fn->name = NULL;
    }

    // 2. Liberar recursos específicos según el tipo de función
    if (fn->type == FOXY_FUNCTION_USER) {
        // Liberar el bytecode de la función de usuario
        if (fn->as.user.code) {
            free(fn->as.user.code);
            fn->as.user.code = NULL;
        }

        // Liberar el sub-pool de constantes de la función
        if (fn->as.user.constants) {
            for (size_t j = 0; j < fn->as.user.constants_count; j++) {
                FoxyValue *val = &fn->as.user.constants[j];
                
                // Liberación recursiva si hay arreglos u objetos internos
                if (val->type == FOXY_VAL_ARRAY || val->type == FOXY_VAL_OBJECT) {
                    if (val->as.array) {
                        free(val->as.array->data);
                        free(val->as.array);
                        val->as.array = NULL;
                    }
                } 
                // Si el sub-pool contiene funciones anidadas, se liberan recursivamente
                else if (val->type == FOXY_VAL_FUNCTION) {
                    f_function_free(val->as.func);
                    val->as.func = NULL;
                }
            }
            free(fn->as.user.constants);
            fn->as.user.constants = NULL;
        }
    } 
    else if (fn->type == FOXY_FUNCTION_NATIVE) {
        // Las funciones nativas (como las de los .so) apuntan a memoria del sistema (dlopen).
        // No se hace free del puntero de la función, pero sí de estructuras auxiliares si las hubiera.
    }

    // 3. Liberar el contenedor principal de la función
    free(fn);
}

void f_function_add_constant(FoxyFunction *func, FoxyValue value) {
    if (!func || func->type != FOXY_FUNCTION_USER) return;

    // Verificar si necesitamos expandir la capacidad del pool de constantes usando los campos de user
    if (func->as.user.constants_count >= func->as.user.constants_capacity) {
        size_t new_cap = func->as.user.constants_capacity == 0 ? 8 : func->as.user.constants_capacity * 2;
        FoxyValue *new_constants = (FoxyValue *)realloc(func->as.user.constants, sizeof(FoxyValue) * new_cap);
        if (!new_constants) {
            return;
        }
        func->as.user.constants = new_constants;
        func->as.user.constants_capacity = new_cap;
    }

    // Insertar el valor FoxyValue e incrementar el contador
    func->as.user.constants[func->as.user.constants_count++] = value;
}