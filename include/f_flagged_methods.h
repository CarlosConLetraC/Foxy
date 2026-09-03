#ifndef FOXY_FLAGGED_METHODS_H
    #define FOXY_FLAGGED_METHODS_H
    #include <stdint.h>
    #include <stdbool.h>
    #include "f_methods.h"

    // Modificadores de acceso y comportamiento (Estilo Java)
    #define FOXY_METHOD_PUBLIC    (1 << 0)
    #define FOXY_METHOD_PRIVATE   (1 << 1)
    #define FOXY_METHOD_PROTECTED (1 << 2)
    #define FOXY_METHOD_STATIC    (1 << 3)
    #define FOXY_METHOD_FINAL     (1 << 4)
    #define FOXY_METHOD_NATIVE    (1 << 5)

    typedef struct FoxyFlaggedMethod {
        FoxyMethod method;      // El método subyacente (nombre y ejecución)
        uint8_t flags;          // Máscara de bits con los modificadores
    } FoxyFlaggedMethod;

    // Funciones de gestión de métodos con flags
    FoxyFlaggedMethod* f_flagged_method_new_native(const char *name, uint8_t flags, FoxyMethodFunc func);
    FoxyFlaggedMethod* f_flagged_method_new_bytecode(const char *name, uint8_t flags, uint32_t offset);
    void f_flagged_method_free(FoxyFlaggedMethod *fmethod);

    // Helpers de validación de modificadores
    bool f_method_is_public(uint8_t flags);
    bool f_method_is_private(uint8_t flags);
    bool f_method_is_protected(uint8_t flags);
    bool f_method_is_static(uint8_t flags);
    bool f_method_is_final(uint8_t flags);
#endif // FOXY_FLAGGED_METHODS_H