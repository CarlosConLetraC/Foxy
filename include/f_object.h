#ifndef FOXY_OBJECT_H
    #define FOXY_OBJECT_H

    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_class.h"

    typedef struct FoxyField {
        char *name;             // Nombre del atributo
        FoxyValue value;        // Valor tipado seguro
    } FoxyField;

    // Definición completa de la estructura para que f_object.c pueda ver sus campos
    struct FoxyObject {
        FoxyClass *klass;
        int ref_count;
        int marked;
        FoxyField *fields;
        size_t field_count;
        size_t field_capacity;
    };

    typedef struct FoxyObject FoxyObject;

    // Funciones de gestión de instancias
    FoxyObject* f_object_new(FoxyClass *klass);
    void f_object_free(FoxyObject *obj);

    void f_object_set_field(FoxyObject *obj, const char *name, FoxyValue val);
    bool f_object_get_field(FoxyObject *obj, const char *name, FoxyValue *out_val);
#endif // FOXY_OBJECT_H