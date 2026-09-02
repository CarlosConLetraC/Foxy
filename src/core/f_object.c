#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "f_object.h"

FoxyObject* f_object_new(FoxyClass *klass) {
    FoxyObject *obj = (FoxyObject *)calloc(1, sizeof(FoxyObject));
    if (!obj) return NULL;

    obj->klass = klass;
    obj->ref_count = 1;
    obj->marked = 0;
    obj->fields = NULL;
    obj->field_count = 0;
    obj->field_capacity = 0;

    return obj;
}

void f_object_set_field(FoxyObject *obj, const char *name, FoxyValue val) {
    if (!obj || !name) return;

    // 1. Si el atributo ya existe, sobreescribir valor
    for (size_t i = 0; i < obj->field_count; i++) {
        if (obj->fields[i].name && strcmp(obj->fields[i].name, name) == 0) {
            obj->fields[i].value = val;
            return;
        }
    }

    // 2. Expandir capacidad si el arreglo está lleno
    if (obj->field_count >= obj->field_capacity) {
        size_t new_cap = obj->field_capacity == 0 ? 4 : obj->field_capacity * 2;
        FoxyField *new_fields = (FoxyField *)realloc(obj->fields, sizeof(FoxyField) * new_cap);
        if (!new_fields) return;

        obj->fields = new_fields;
        obj->field_capacity = new_cap;
    }

    // 3. Insertar nuevo atributo
    size_t len = strlen(name);
    char *name_copy = (char *)malloc(len + 1);
    if (!name_copy) return;
    memcpy(name_copy, name, len + 1);

    obj->fields[obj->field_count].name = name_copy;
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

    return false; // Campo no encontrado
}

void f_object_free(FoxyObject *obj) {
    if (!obj) return;

    for (size_t i = 0; i < obj->field_count; i++) {
        if (obj->fields[i].name) {
            free(obj->fields[i].name);
        }
    }
    if (obj->fields) {
        free(obj->fields);
    }
    free(obj);
}