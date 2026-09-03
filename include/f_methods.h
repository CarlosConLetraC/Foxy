#ifndef FOXY_METHODS_H
    #define FOXY_METHODS_H

    #include <stdint.h>
    #include <stddef.h>

    /* Firma para funciones nativas C del runtime */
    typedef void (*FoxyMethodFunc)(void);

    /* X-Macro para los tipos de métodos */
    #define FOXY_METHOD_TYPE_LIST(F) \
        F(FOXY_METHOD_TYPE_NATIVE,   "NATIVE") \
        F(FOXY_METHOD_TYPE_BYTECODE, "BYTECODE")

    #define F(type, name) type,
    typedef enum __attribute__((__packed__)) {
        FOXY_METHOD_TYPE_LIST(F)
        FOXY_METHOD_TYPE_COUNT
    } FoxyMethodType;
    #undef F

    typedef struct FoxyMethod {
        const char *name;
        FoxyMethodType type;
        union {
            FoxyMethodFunc native_func;
            uint32_t bytecode_offset;
        } as;
    } FoxyMethod;

    /* Inicializadores y destructores para FoxyMethod */
    void f_method_init_native(FoxyMethod *method, const char *name, FoxyMethodFunc func);
    void f_method_init_bytecode(FoxyMethod *method, const char *name, uint32_t offset);
    void f_method_free(FoxyMethod *method);

    /* Prototipo del helper */
    const char* f_method_type_to_string(FoxyMethodType type);
#endif // FOXY_METHODS_H