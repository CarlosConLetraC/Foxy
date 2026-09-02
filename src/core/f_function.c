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

    if (func->name)
        free(func->name);

    if (func->code)
        free(func->code);

    // Liberar las constantes internas asignadas a la función
    if (func->constants) {
        for (size_t i = 0; i < func->constants_count; i++) {
            if (func->constants[i].type == FOXY_CONSTANT_CHARARRAY && func->constants[i].as.sval)
                free(func->constants[i].as.sval);
        }
        free(func->constants);
    }

    free(func);
}