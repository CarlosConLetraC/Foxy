#ifndef F_FUNCTION_H
    #define F_FUNCTION_H

    #include <stdint.h>
    #include <stddef.h>
    #include "f_value.h"
    #include "f_foxcode.h"
    #include "f_foxmode.h"

    typedef struct FoxyVM FoxyVM;
    typedef struct FoxyProcess FoxyProcess;
    typedef struct FoxyEnv FoxyEnv;

    typedef void (*FoxyNativeFn)(FoxyVM *vm, FoxyProcess *proc, int arg_count);

    #define FOXY_FUNCTION_TYPE_LIST(F) \
        F(FOXY_FUNCTION_USER,   "foxy")   \
        F(FOXY_FUNCTION_NATIVE, "C")

    typedef enum {
        #define F(type_enum, type_str) type_enum,
        FOXY_FUNCTION_TYPE_LIST(F)
        #undef F
        FOXY_FUNC_TYPE_COUNT
    } FoxyFunctionType;

    extern const char * const FOXY_FUNCTION_TYPE_NAMES[];

    struct FoxyFunction {
        char *name;
        uint8_t arity;
        FoxyFunctionType type;

        union {
            struct {
                FoxmodeInstruction *code;
                size_t code_size;
                size_t code_capacity;

                size_t locals_count;
                size_t locals_capacity;

                FoxyValue *constants;
                size_t constants_count;
                size_t constants_capacity;

                FoxyEnv *env;
            } user;

            struct {
                FoxyNativeFn function_ptr;
            } native;
        } as;
    };

    FoxyFunction* f_function_create(const char *name, uint8_t arity);
    FoxyFunction* f_function_create_native(const char *name, uint8_t arity, FoxyNativeFn native_ptr);
    void f_function_free(FoxyFunction *func, FoxyVM *vm);
    void f_function_add_constant(FoxyFunction *func, FoxyValue value);
#endif // F_FUNCTION_H