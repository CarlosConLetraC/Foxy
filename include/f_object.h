#ifndef FOXY_OBJECT_H
    #define FOXY_OBJECT_H

    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include "f_value.h" // Asegurar que conoce FoxyValue
    #include "f_class.h"

    typedef struct FoxyField {
        char *name;             // Nombre del atributo (es mejor char* dinámico si se asigna en runtime)
        FoxyValue value;        // Usar FoxyValue en lugar de uint64_t crudo para tipado seguro
    } FoxyField;

    struct FoxyObject {
        FoxyClass *klass;          // Puntero a la clase a la que pertenece
        FoxyField *fields;         // Atributos de la instancia
        size_t field_count;
        size_t field_capacity;
        bool is_marked;            // Flag útil para el Recolector de Basura (GC)
    };

    // Funciones de gestión de instancias
    FoxyObject* f_object_new(FoxyClass *klass);
    void f_object_free(FoxyObject *obj);

    // Métodos actualizados para trabajar con FoxyValue directamente
    void f_object_set_field(FoxyObject *obj, const char *name, FoxyValue val);
    bool f_object_get_field(FoxyObject *obj, const char *name, FoxyValue *out_val);
#endif // FOXY_OBJECT_H