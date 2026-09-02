#ifndef FOXY_METHODS_H
    #define FOXY_METHODS_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "f_vm.h"

    typedef struct FoxyObject FoxyObject;

    // Puntero a función nativa asociada a un método
    typedef void (*FoxyMethodFunc)(FoxyVM *vm, FoxyObject *self, int argc);

    typedef struct FoxyMethod {
        char *name;
        bool is_native;
        union {
            FoxyMethodFunc native_fn;
            uint32_t bytecode_offset; // Para métodos definidos en código Foxy
        } as;
    } FoxyMethod;

    // Constructores y destructores de métodos
    FoxyMethod* f_method_new_native(const char *name, FoxyMethodFunc func);
    FoxyMethod* f_method_new_bytecode(const char *name, uint32_t offset);
    void f_method_free(FoxyMethod *method);
#endif // FOXY_METHODS_H