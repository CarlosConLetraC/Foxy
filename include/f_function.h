#ifndef F_FUNCTION_H
    #define F_FUNCTION_H

    #include <stdint.h>
    #include <stddef.h>
    #include "f_constant.h"
    #include "f_value.h"
    #include "f_env.h"

    // Declaraciones adelantadas para evitar dependencias circulares
    typedef struct FoxyVM FoxyVM;
    typedef struct FoxyProcess FoxyProcess;

    // Prototipo estándar para funciones nativas en C
    typedef void (*FoxyNativeFn)(FoxyVM *vm, FoxyProcess *proc, int arg_count);

    // Discriminador para saber cómo ejecutar la función
    #define FOXY_FUNCTION_TYPE_LIST(F) \
        F(FOXY_FUNCTION_USER,   "foxy")   \
        F(FOXY_FUNCTION_NATIVE, "C")

    // Generación automática del enum FoxyFunctionType y su centinela
    typedef enum {
        #define F(type_enum, type_str) type_enum,
        FOXY_FUNCTION_TYPE_LIST(F)
        #undef F
        FOXY_FUNC_TYPE_COUNT
    } FoxyFunctionType;

    // Solo la declaración externa (sin valores aquí)
    extern const char *FOXY_FUNCTION_TYPE_NAMES[];

    struct FoxyFunction {
        char *name;
        uint8_t arity;
        FoxyFunctionType type; // <--- Identifica si es nativa o de usuario

        union {
            // Datos exclusivos para funciones de usuario
            struct {
                uint8_t *code;
                size_t code_size;
                size_t code_capacity;

                size_t locals_count;
                size_t locals_capacity;

                FoxyValue *constants;
                size_t constants_count;
                size_t constants_capacity;

                FoxyEnv *env; // Entorno léxico
            } user;

            // Datos exclusivos para funciones nativas de C
            struct {
                FoxyNativeFn function_ptr;
            } native;
        } as;
    };

    void f_function_free(FoxyFunction *func);
#endif // F_FUNCTION_H