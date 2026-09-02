#ifndef FOXY_VM_H
    #define FOXY_VM_H

    #include "f_value.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>

    // Forward declarations de estructuras exclusivas de la VM
    // typedef struct FoxyVM FoxyVM;
    typedef struct FoxyProcess FoxyProcess;

    // Firma de funciones nativas aceptando el objeto emisor (self)
    typedef FoxyValue (*FoxyNativeMethod)(int arg_count, FoxyValue* args);

    typedef enum {
        FOXY_PROCESS_READY,
        FOXY_PROCESS_RUNNING,
        FOXY_PROCESS_DEAD
    } FoxyProcessState;

    struct FoxyProcess {
        uint32_t pid;
        char name[64];
        FoxyProcessState state;
        
        uint8_t *bytecode;
        size_t bytecode_size;
        size_t ip;

        // Usamos directamente FoxyValue de f_value.h
        FoxyValue *stack;
        size_t stack_top;
        size_t stack_capacity;
    };

    typedef struct {
        char name[64];
        FoxyNativeMethod func;
    } FoxyNativeSymbol;

    typedef struct FoxyVM {
        // Símbolos nativos (Funciones C registradas)
        FoxyNativeSymbol* native_symbols;
        size_t native_symbols_count;
        size_t native_symbols_capacity;
        
        // Librerías dinámicas cargadas
        char** loaded_libs;
        size_t loaded_libs_count;
        size_t loaded_libs_capacity;
        
        // Constant Pool usa el sistema de tipos unificado
        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;
        
        FoxyProcess **processes;
        size_t process_count;
        size_t process_capacity;
        size_t current_process_index;

        FoxyClass **classes;
        size_t class_count;

        FoxyObject *objects_head;
        bool running;
    } FoxyVM;

    // Operaciones del Stack en la VM
    int f_vm_run(FoxyVM* vm);
    void f_vm_load_process(FoxyVM* vm, const uint8_t* code, size_t code_size, const char* filename);
    void f_vm_register_native(FoxyVM* vm, const char* name, FoxyNativeMethod func);
    void f_vm_push(FoxyProcess *p, FoxyValue val);
    void f_vm_free(FoxyVM* vm);
    FoxyVM* f_vm_create(void);
    FoxyValue f_vm_pop(FoxyProcess *p);
    FoxyValue f_vm_peek(FoxyProcess *p, size_t distance);
    FoxyNativeMethod f_vm_find_native(FoxyVM* vm, const char* name);
    FoxyValue f_vm_stack_peek(FoxyProcess *p, size_t dist);
    
    void f_process_push(FoxyProcess *p, FoxyValue value);
    FoxyValue f_process_pop(FoxyProcess *p);
#endif // FOXY_VM_H