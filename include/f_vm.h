#ifndef F_VM_H
    #define F_VM_H

    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include "f_value.h"
    #include "f_process.h"

    typedef struct FoxyRuntime FoxyRuntime;
    typedef struct FoxyObject FoxyObject;

    typedef void (*FoxyNativeFunc)(FoxyVM *vm, FoxyObject *obj, int args);
    typedef FoxyNativeFunc FoxyNativeMethod;

    typedef struct {
        char name[128];
        FoxyNativeFunc func;
    } FoxyNativeSymbol;

    typedef struct FoxyVM {
        FoxyRuntime *runtime;
        bool running;

        // Process table
        FoxyProcess **processes;
        size_t process_count;
        size_t process_capacity;
        size_t current_process_index;

        // Constant Pool
        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;

        // Native Function Table
        FoxyNativeSymbol *native_symbols;
        size_t native_symbols_count;
        size_t native_symbols_capacity;

        // Loaded Libraries Tracking (para f_openlib.c)
        char **loaded_libs;
        size_t loaded_libs_count;
        size_t loaded_libs_capacity;
    } FoxyVM;

    FoxyVM* f_vm_new(void);
    void f_vm_free(FoxyVM *vm);
    void f_vm_push(FoxyProcess *p, FoxyValue val);
    void f_vm_register_native(FoxyVM *vm, const char *name, FoxyNativeMethod func);
    
    FoxyValue f_vm_pop(FoxyProcess *p);
    FoxyValue f_vm_peek(FoxyProcess *p, size_t distance);
    FoxyNativeFunc f_vm_find_native(FoxyVM *vm, const char *name);

    void f_vm_load_process(FoxyVM *vm, const uint8_t *code, size_t code_size, const char *filename);
    int f_vm_run(FoxyVM *vm);
#endif // F_VM_H