#ifndef F_OBJECT_H
    #define F_OBJECT_H

    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_class.h"

    typedef struct FoxyField {
        char *name;
        FoxyValue value;
    } FoxyField;

    struct FoxyObject {
        FoxyClass *klass;
        size_t ref_count;
        int marked;
        FoxyField *fields;
        size_t field_count;
        size_t field_capacity;
    };

    FoxyObject* f_object_new(FoxyClass *klass);
    void f_object_free(FoxyObject *obj);
    void f_object_set_field(FoxyObject *obj, const char *name, FoxyValue val);
    bool f_object_get_field(FoxyObject *obj, const char *name, FoxyValue *out_val);
#endif