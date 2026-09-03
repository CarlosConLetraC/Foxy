#ifndef F_FUNCTION_H
    #define F_FUNCTION_H

    #include <stdint.h>
    #include <stddef.h>
    #include "f_constant.h" // Asegúrate de incluir la definición de FoxyConstant
    #include "f_value.h"

    struct FoxyFunction {
        char *name;
        uint8_t arity;
        
        // Nombres alternativos o alias de bytecode usados por f_function.c
        uint8_t *code;
        size_t code_size;
        size_t code_capacity;

        size_t bytecode_size;
        uint8_t *bytecode;

        size_t locals_count;
        size_t locals_capacity;

        // Pool de constantes asociadas a la función
        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;
    };

    void f_function_free(FoxyFunction *func);
#endif // F_FUNCTION_H