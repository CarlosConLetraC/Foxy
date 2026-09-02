#include "f_flagged_methods.h"
#include <stdlib.h>
#include <string.h>

FoxyFlaggedMethod* f_flagged_method_new_native(const char *name, uint8_t flags, FoxyMethodFunc func) {
    FoxyFlaggedMethod *fm = (FoxyFlaggedMethod*)malloc(sizeof(FoxyFlaggedMethod));
    if (!fm) return NULL;

    fm->method.name = strdup(name);
    fm->method.is_native = true;
    fm->method.as.native_fn = func;
    fm->flags = flags | FOXY_METHOD_NATIVE; // Asegurar consistencia en la flag
    return fm;
}

FoxyFlaggedMethod* f_flagged_method_new_bytecode(const char *name, uint8_t flags, uint32_t offset) {
    FoxyFlaggedMethod *fm = (FoxyFlaggedMethod*)malloc(sizeof(FoxyFlaggedMethod));
    if (!fm) return NULL;

    fm->method.name = strdup(name);
    fm->method.is_native = false;
    fm->method.as.bytecode_offset = offset;
    fm->flags = flags & ~FOXY_METHOD_NATIVE; // No nativo
    return fm;
}

void f_flagged_method_free(FoxyFlaggedMethod *fmethod) {
    if (!fmethod) return;
    free(fmethod->method.name);
    free(fmethod);
}

bool f_method_is_public(uint8_t flags) {
    return (flags & FOXY_METHOD_PUBLIC) != 0;
}

bool f_method_is_private(uint8_t flags) {
    return (flags & FOXY_METHOD_PRIVATE) != 0;
}

bool f_method_is_static(uint8_t flags) {
    return (flags & FOXY_METHOD_STATIC) != 0;
}

bool f_method_is_final(uint8_t flags) {
    return (flags & FOXY_METHOD_FINAL) != 0;
}