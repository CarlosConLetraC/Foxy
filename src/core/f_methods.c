#include "f_methods.h"
#include <stdlib.h>
#include <string.h>

FoxyMethod* f_method_new_native(const char *name, FoxyMethodFunc func) {
    FoxyMethod *m = (FoxyMethod*)malloc(sizeof(FoxyMethod));
    if (!m) return NULL;
    
    m->name = strdup(name);
    m->is_native = true;
    m->as.native_fn = func;
    return m;
}

FoxyMethod* f_method_new_bytecode(const char *name, uint32_t offset) {
    FoxyMethod *m = (FoxyMethod*)malloc(sizeof(FoxyMethod));
    if (!m) return NULL;
    
    m->name = strdup(name);
    m->is_native = false;
    m->as.bytecode_offset = offset;
    return m;
}

void f_method_free(FoxyMethod *method) {
    if (!method) return;
    free(method->name);
    free(method);
}