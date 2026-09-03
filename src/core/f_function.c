#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_function.h"

FoxyFunction* f_function_create(const char *name, uint8_t arity) {
    FoxyFunction *func = (FoxyFunction*) malloc(sizeof(FoxyFunction));
    if (!func) return NULL;

    func->name = name ? strdup(name) : NULL;
    func->arity = arity;
    
    func->code = NULL;
    func->code_size = 0;
    func->code_capacity = 0;

    func->constants = NULL;
    func->constants_count = 0;
    func->constants_capacity = 0;

    func->locals_count = 0;

    return func;
}

void f_function_free(FoxyFunction *func) {
    if (!func) return;

    if (func->name) {
        free(func->name);
        func->name = NULL;
    }

    if (func->bytecode) {
        free(func->bytecode);
        func->bytecode = NULL;
    }

    if (func->constants) {
        for (size_t i = 0; i < func->constants_count; i++) {
            f_value_free_contents(&func->constants[i]);
        }
        free(func->constants);
        func->constants = NULL;
    }

    free(func);
}

void f_function_add_constant(FoxyFunction *func, FoxyValue value) {
    if (!func) return;

    // Verificar si necesitamos expandir la capacidad del pool de constantes de la función
    if (func->constants_count >= func->constants_capacity) {
        size_t new_cap = func->constants_capacity == 0 ? 8 : func->constants_capacity * 2;
        FoxyConstant *new_constants = (FoxyConstant *)realloc(func->constants, sizeof(FoxyConstant) * new_cap);
        if (!new_constants) {
            // Manejo básico de error de asignación (puedes loguear o abortar según prefieras)
            return;
        }
        func->constants = new_constants;
        func->constants_capacity = new_cap;
    }

    // Insertar el valor e incrementar el contador
    func->constants[func->constants_count++] = value;
}