#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_class.h"
#include "f_gc.h"

FoxyClass* f_class_new(const char *name, FoxyClass *super_class) {
    FoxyClass *klass = (FoxyClass*)f_gc_allocate(FOXY_HEAP_CLASS, sizeof(FoxyClass), (void(*)(void*))f_class_free);
    if (!klass) return NULL;

    klass->name = name ? strdup(name) : NULL;
    klass->super_class = super_class;
    klass->methods = NULL;
    klass->method_count = 0;
    klass->method_capacity = 0;

    return klass;
}

void f_class_add_method_native(FoxyClass *klass, const char *name, FoxyMethodFunc func) {
    if (!klass) return;

    if (klass->method_count >= klass->method_capacity) {
        size_t new_capacity = klass->method_capacity ? klass->method_capacity * 2 : 4;
        FoxyMethod *new_methods = (FoxyMethod*)realloc(klass->methods, sizeof(FoxyMethod) * new_capacity);
        if (!new_methods) return;

        klass->methods = new_methods;
        klass->method_capacity = new_capacity;
    }

    f_method_init_native(&klass->methods[klass->method_count], name, func);
    klass->method_count++;
}

FoxyMethod* f_class_find_method(FoxyClass *klass, const char *name) {
    if (!klass || !name) return NULL;

    FoxyClass *current = klass;
    while (current) {
        for (size_t i = 0; i < current->method_count; i++) {
            if (current->methods[i].name && strcmp(current->methods[i].name, name) == 0) {
                return &current->methods[i];
            }
        }
        current = current->super_class;
    }

    return NULL;
}

void f_class_free(FoxyClass *klass) {
    if (!klass) return;

    if (klass->name) {
        free((void*)klass->name);
    }

    for (size_t i = 0; i < klass->method_count; i++) {
        f_method_free(&klass->methods[i]);
    }

    if (klass->methods) {
        free(klass->methods);
    }
}