#include "f_settings.h"
#include <string.h>
#include <stdlib.h>
#include "f_class.h"

FoxyClass* f_class_new(const char *name, FoxyClass *super_class) {
    FoxyClass *klass = (FoxyClass*)malloc(sizeof(FoxyClass));
    if (!klass) return NULL;

    klass->name = strdup(name);
    klass->super_class = super_class;
    klass->methods = NULL;
    klass->method_count = 0;
    klass->method_capacity = 0;
    return klass;
}

void f_class_add_method_native(FoxyClass *klass, const char *name, FoxyMethodFunc func) {
    if (!klass) return;
    if (klass->method_count >= klass->method_capacity) {
        klass->method_capacity = klass->method_capacity ? klass->method_capacity * 2 : 4;
        klass->methods = (FoxyMethod*)realloc(klass->methods, sizeof(FoxyMethod) * klass->method_capacity);
    }

    klass->methods[klass->method_count].name = strdup(name);
    klass->methods[klass->method_count].is_native = true;
    klass->methods[klass->method_count].as.native_fn = func;
    klass->method_count++;
}

FoxyMethod* f_class_find_method(FoxyClass *klass, const char *name) {
    FoxyClass *current = klass;
    while (current != NULL) {
        for (size_t i = 0; i < current->method_count; i++) {
            if (strcmp(current->methods[i].name, name) == 0) {
                return &current->methods[i];
            }
        }
        current = current->super_class; // Búsqueda en la jerarquía
    }
    return NULL;
}

void f_class_free(FoxyClass *klass) {
    if (!klass) return;
    free((void*)klass->name);
    for (size_t i = 0; i < klass->method_count; i++) {
        free((void*)klass->methods[i].name);
    }
    free(klass->methods);
    free(klass);
}