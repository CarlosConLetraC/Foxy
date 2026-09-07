#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_function.h"
#include "f_gc.h"

const char * const FOXY_FUNCTION_TYPE_NAMES[] = {
    #define X(type_enum, type_str) [type_enum] = type_str,
    FOXY_FUNCTION_TYPE_LIST(X)
    #undef X
};

FoxyFunction* f_function_create(const char *name, uint8_t arity) {
    FoxyFunction *func = (FoxyFunction*)f_gc_allocate(FOXY_HEAP_FUNCTION, sizeof(FoxyFunction), (void(*)(void*))f_function_free);
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

    if (fn->name) {
        free(fn->name);
        fn->name = NULL;
    }

    if (fn->type == FOXY_FUNCTION_USER) {
        if (fn->as.user.code) {
            free(fn->as.user.code);
            fn->as.user.code = NULL;
        }
        if (fn->as.user.constants) {
            for (size_t i = 0; i < fn->as.user.constants_count; i++) {
                f_value_free_contents(&fn->as.user.constants[i]);
            }
            free(fn->as.user.constants);
            fn->as.user.constants = NULL;
        }
    }

    free(fn);
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