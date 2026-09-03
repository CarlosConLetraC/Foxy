#ifndef F_CLASS_H
    #define F_CLASS_H

    #include <stddef.h>
    #include "f_methods.h" // Importa FoxyMethod, FoxyMethodFunc y dependencias
    #include "f_process.h"
    #include "f_vm.h"

    // Estructura de la Clase
    typedef struct FoxyClass {
        const char *name;
        struct FoxyClass *super_class;  // Herencia
        FoxyMethod *methods;             // Tabla de métodos (de f_methods.h)
        size_t method_count;
        size_t method_capacity;
    } FoxyClass;

    // Funciones del subsistema de clases
    FoxyClass* f_class_new(const char *name, FoxyClass *super_class);
    void f_class_add_method_native(FoxyClass *klass, const char *name, FoxyMethodFunc func);
    FoxyMethod* f_class_find_method(FoxyClass *klass, const char *name);
    void f_class_free(FoxyClass *klass);
#endif // F_CLASS_H