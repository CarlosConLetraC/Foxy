#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_object.h"
#include "f_gc.h"

static void f_object_gc_free(void *ptr) {
    if (!ptr) return;
    f_object_free((FoxyObject *)ptr, NULL);
}

FoxyObject* f_object_new(FoxyClass *klass) {
    FoxyObject *obj = (FoxyObject*)f_gc_allocate(FOXY_HEAP_OBJECT, sizeof(FoxyObject), f_object_gc_free);
    if (!obj) return NULL;

    obj->klass = klass;
    obj->fields = NULL;
    obj->field_count = 0;
    obj->field_capacity = 0;

    return obj;
}

void f_object_free(FoxyObject *obj, FoxyVM *vm) {
    if (!obj) return;
    for (size_t i = 0; i < obj->field_count; i++) {
        if (obj->fields[i].name) free(obj->fields[i].name);
        f_value_free_contents(&obj->fields[i].value, vm);
    }
    if (obj->fields) free(obj->fields);
    free(obj);
}

void f_object_set_field(FoxyObject *obj, const char *name, FoxyValue val, FoxyVM *vm) {
    if (!obj || !name) return;

    for (size_t i = 0; i < obj->field_count; i++) {
        if (obj->fields[i].name && strcmp(obj->fields[i].name, name) == 0) {
            f_value_free_contents(&obj->fields[i].value, vm);
            obj->fields[i].value = val;
            return;
        }
    }

    if (obj->field_count >= obj->field_capacity) {
        size_t new_cap = obj->field_capacity == 0 ? 4 : obj->field_capacity * 2;
        FoxyField *new_fields = (FoxyField*)realloc(obj->fields, sizeof(FoxyField) * new_cap);
        if (!new_fields) return;

        obj->fields = new_fields;
        obj->field_capacity = new_cap;
    }

    obj->fields[obj->field_count].name = strdup(name);
    obj->fields[obj->field_count].value = val;
    obj->field_count++;
}

bool f_object_get_field(FoxyObject *obj, const char *name, FoxyValue *out_val) {
    if (!obj || !name || !out_val) return false;

    for (size_t i = 0; i < obj->field_count; i++) {
        if (obj->fields[i].name && strcmp(obj->fields[i].name, name) == 0) {
            *out_val = obj->fields[i].value;
            return true;
        }
    }

    return false;
}