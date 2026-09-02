#include "f_object.h"
#include <stdlib.h>
#include <string.h>

FoxyObject* f_object_new(FoxyClass *klass) {
    FoxyObject *obj = (FoxyObject*)malloc(sizeof(FoxyObject));
    if (!obj) return NULL;

    obj->klass = klass;
    obj->fields = NULL;
    obj->field_count = 0;
    obj->field_capacity = 0;
    obj->is_marked = false;
    return obj;
}

void f_object_set_field(FoxyObject *obj, const char *name, uint64_t val) {
    if (!obj) return;
    
    // Buscar si ya existe la propiedad para sobrescribirla
    for (size_t i = 0; i < obj->field_count; i++) {
        if (strcmp(obj->fields[i].name, name) == 0) {
            obj->fields[i].value = val;
            return;
        }
    }

    if (obj->field_count >= obj->field_capacity) {
        obj->field_capacity = obj->field_capacity ? obj->field_capacity * 2 : 4;
        obj->fields = (FoxyField*)realloc(obj->fields, sizeof(FoxyField) * obj->field_capacity);
    }

    obj->fields[obj->field_count].name = strdup(name);
    obj->fields[obj->field_count].value = val;
    obj->field_count++;
}

bool f_object_get_field(FoxyObject *obj, const char *name, uint64_t *out_val) {
    if (!obj) return false;
    for (size_t i = 0; i < obj->field_count; i++) {
        if (strcmp(obj->fields[i].name, name) == 0) {
            if (out_val) *out_val = obj->fields[i].value;
            return true;
        }
    }
    return false;
}

void f_object_free(FoxyObject *obj) {
    if (!obj) return;
    for (size_t i = 0; i < obj->field_count; i++) {
        free((void*)obj->fields[i].name);
    }
    free(obj->fields);
    free(obj);
}