#ifndef FOXY_CLASS_H
    #define FOXY_CLASS_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "f_vm.h"

    typedef struct FoxyClass FoxyClass;
    typedef struct FoxyObject FoxyObject;

    // Puntero a función nativa asociada a un método
    typedef void (*FoxyMethodFunc)(FoxyVM *vm, FoxyObject *self, int argc);

    typedef struct FoxyMethod {
        const char *name;
        bool is_native;
        union {
            FoxyMethodFunc native_fn;
            uint32_t bytecode_offset; // Para métodos definidos en código Foxy
        } as;
    } FoxyMethod;

    struct FoxyClass {
        const char *name;
        FoxyClass *super_class;
        
        // Tabla de métodos (VTable simple)
        FoxyMethod *methods;
        size_t method_count;
        size_t method_capacity;
    };

    // Funciones de gestión de clases
    FoxyClass* f_class_new(const char *name, FoxyClass *super_class);
    void f_class_free(FoxyClass *klass);
    void f_class_add_method_native(FoxyClass *klass, const char *name, FoxyMethodFunc func);
    FoxyMethod* f_class_find_method(FoxyClass *klass, const char *name);
#endif // FOXY_CLASS_H