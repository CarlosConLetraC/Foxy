#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_function.h"
#include "f_gc.h"
#include "f_vm.h" // Se incluye para resolver la estructura completa de FoxyVM

const char * const FOXY_FUNCTION_TYPE_NAMES[] = {
    #define X(type_enum, type_str) [type_enum] = type_str,
    FOXY_FUNCTION_TYPE_LIST(X)
    #undef X
};

// Wrapper para adaptar la firma al destructor esperado por el GC: void (*)(void *)
static void f_function_gc_free(void *ptr) {
    if (!ptr) return;
    f_function_free((FoxyFunction *)ptr, NULL);
}

FoxyFunction* f_function_create(const char *name, uint8_t arity) {
    // Se pasa el wrapper f_function_gc_free para eliminar el warning de cast incompatible
    FoxyFunction *func = (FoxyFunction*)f_gc_allocate(FOXY_HEAP_FUNCTION, sizeof(FoxyFunction), f_function_gc_free);
    if (!func) return NULL;

    func->name = name ? strdup(name) : NULL;
    func->arity = arity;
    func->type = FOXY_FUNCTION_USER;

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

FoxyFunction* f_function_create_native(const char *name, uint8_t arity, FoxyNativeFn native_ptr) {
    FoxyFunction *func = (FoxyFunction*)f_gc_allocate(FOXY_HEAP_FUNCTION, sizeof(FoxyFunction), f_function_gc_free);
    if (!func) return NULL;

    func->name = name ? strdup(name) : NULL;
    func->arity = arity;
    func->type = FOXY_FUNCTION_NATIVE;
    func->as.native.function_ptr = native_ptr;

    return func;
}

void f_function_free(FoxyFunction *func, FoxyVM *vm) {
    if (!func) return;

    if (func->name) {
        free((void *)func->name);
        func->name = NULL;
    }

    if (func->type == FOXY_FUNCTION_USER) {
        // Solo liberar constants si no es el puntero global compartido de la VM
        if (func->as.user.constants && (!vm || func->as.user.constants != vm->constants)) {
            free(func->as.user.constants);
            func->as.user.constants = NULL;
        }

        if (func->as.user.code) {
            free((void *)func->as.user.code);
            func->as.user.code = NULL;
        }
    }

    free(func);
}

void f_function_add_constant(FoxyFunction *func, FoxyValue value) {
    if (!func || func->type != FOXY_FUNCTION_USER) return;

    if (func->as.user.constants_count >= func->as.user.constants_capacity) {
        size_t new_cap = func->as.user.constants_capacity == 0 ? 8 : func->as.user.constants_capacity * 2;
        FoxyValue *new_constants = (FoxyValue*)realloc(func->as.user.constants, sizeof(FoxyValue) * new_cap);
        if (!new_constants) return;

        func->as.user.constants = new_constants;
        func->as.user.constants_capacity = new_cap;
    }

    func->as.user.constants[func->as.user.constants_count++] = value;
}