#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_flagged_methods.h"

FoxyFlaggedMethod* f_flagged_method_new_native(const char *name, uint8_t flags, FoxyMethodFunc func) {
    FoxyFlaggedMethod *fm = (FoxyFlaggedMethod*) malloc(sizeof(FoxyFlaggedMethod));
    if (!fm) return NULL;

    fm->flags = flags | FOXY_METHOD_NATIVE;
    fm->method.name = name;
    fm->method.type = FOXY_METHOD_TYPE_NATIVE;
    fm->method.as.native_func = func;

    return fm;
}

FoxyFlaggedMethod* f_flagged_method_new_bytecode(const char *name, uint8_t flags, uint32_t offset) {
    FoxyFlaggedMethod *fm = (FoxyFlaggedMethod*) malloc(sizeof(FoxyFlaggedMethod));
    if (!fm) return NULL;

    fm->flags = flags;
    fm->method.name = name;
    fm->method.type = FOXY_METHOD_TYPE_BYTECODE;
    fm->method.as.bytecode_offset = offset;

    return fm;
}

void f_flagged_method_free(FoxyFlaggedMethod *fmethod) {
    if (!fmethod) return;

    if (fmethod->method.name) {
        free((void*) fmethod->method.name);
    }

    free(fmethod);
}

bool f_method_is_public(uint8_t flags)    { return (flags & FOXY_METHOD_PUBLIC) != 0; }
bool f_method_is_private(uint8_t flags)   { return (flags & FOXY_METHOD_PRIVATE) != 0; }
bool f_method_is_protected(uint8_t flags) { return (flags & FOXY_METHOD_PROTECTED) != 0; }
bool f_method_is_static(uint8_t flags)    { return (flags & FOXY_METHOD_STATIC) != 0; }
bool f_method_is_final(uint8_t flags)     { return (flags & FOXY_METHOD_FINAL) != 0; }