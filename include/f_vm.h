#ifndef F_VM_H
    #define F_VM_H

    #include "f_settings.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include "uthash.h"
    #include "f_value.h"
    #include "f_runtime.h"
    #include "f_symtable.h"
    #include "f_lib.h"
    #include "f_methods.h"
    #include "f_status.h"
    #include "f_process.h"

    typedef void (*FoxyNativeFunc)(FoxyVM *vm, FoxyObject *obj, int args);
    typedef FoxyNativeFunc FoxyNativeMethod;

    // Nodo Hash para símbolos/métodos nativos en C
    typedef struct FoxyNativeSymbolEntry {
        char name[FOXY_MAX_IDENTIFIER_LEN]; // Clave (Key)
        FoxyNativeMethod func;               // Valor (Función C)
        UT_hash_handle hh;                   // Handle interno de uthash
    } FoxyNativeSymbolEntry;

    typedef struct FoxyVM {
        FoxyRuntime *runtime;
        bool running;

        // Tabla Hash de Procesos (indexada por PID)
        FoxyProcess *processes_hash;

        // Pool de constantes (Valores Foxy de runtime)
        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;

        // Tabla Hash de Símbolos y Protocolos Nativos de C
        FoxyNativeSymbolEntry *native_symbols_hash;

        // Cargador de librerías y subsistema de símbolos
        FoxySymbolTable *symtable; 
        FoxyLib *loading_lib;
        FoxyMethod *method;
        char **loaded_libs;
        size_t loaded_libs_count;
        size_t loaded_libs_capacity;

        FoxyValue *globals;
        size_t globals_count;
        size_t globals_capacity;
    } FoxyVM;

    FoxyVM* f_vm_new(void);
    void f_vm_free(FoxyVM *vm);
    void f_vm_push(FoxyProcess *p, FoxyValue val);

    // Búsqueda y Registro O(1) de Protocolos Nativos de C
    FoxyNativeMethod f_vm_lookup_native(FoxyVM *vm, const char *name);
    void f_vm_register_native(FoxyVM *vm, const char *name, FoxyNativeMethod func);

    // Gestión de Procesos vía uthash
    FoxyProcess* f_vm_get_process(FoxyVM *vm, FoxyProcess *proc_ptr);
    void f_vm_add_process(FoxyVM *vm, FoxyProcess *proc);

    void f_vm_load_module(FoxyVM *vm, const char *path);
    FoxyStatus f_vm_execute_process(FoxyVM *vm, FoxyProcess *proc);

    FoxyValue f_vm_pop(FoxyProcess *p);
    FoxyValue f_vm_peek(FoxyProcess *p, size_t distance);
    FoxyLib* f_vm_get_current_loading_lib(FoxyVM *vm);
    void f_vm_set_current_loading_lib(FoxyVM *vm, FoxyLib *lib);

    void f_vm_load_process(FoxyVM *vm, const uint8_t *code, size_t code_size, const char *filename);
    void f_vm_stack_pop_n(FoxyVM *vm, size_t n);
    FoxyStatus f_vm_run(FoxyVM *vm);
#endif // F_VM_H