#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_methods.h"

void f_method_init_native(FoxyMethod *method, const char *name, FoxyMethodFunc func) {
    if (!method) return;
    method->name = name;
    method->type = FOXY_METHOD_TYPE_NATIVE;
    method->as.native_func = func;
}

void f_method_init_bytecode(FoxyMethod *method, const char *name, uint32_t offset) {
    if (!method) return;
    method->name = name;
    method->type = FOXY_METHOD_TYPE_BYTECODE;
    method->as.bytecode_offset = offset;
}

void f_method_free(FoxyMethod *method) {
    if (!method) return;

    if (method->name) {
        free((void*) method->name);
        method->name = NULL;
    }
}

const char* f_method_type_to_string(FoxyMethodType type) {
    switch (type) {
#define F(t, name) case t: return name;
        FOXY_METHOD_TYPE_LIST(F)
#undef F
        default: return "UNKNOWN";
    }
}