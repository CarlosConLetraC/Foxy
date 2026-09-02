#ifndef F_VM_H
    #define F_VM_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "f_foxcode.h"
    #include "f_parser.h"

    // Estados posibles para los procesos del Task Manager interno
    typedef enum __attribute__((__packed__)) {
        FOXY_PROCESS_READY,
        FOXY_PROCESS_RUNNING,
        FOXY_PROCESS_WAITING,
        FOXY_PROCESS_DEAD
    } FOXY_PROCESS_STATE;

    // Foxy-constant soportadas
    typedef enum __attribute__((__packed__)) {
        FOXY_CONSTANT_INT,
        FOXY_CONSTANT_CHARARRAY
    } FOXY_CONSTANT_TYPE;

    // Estructura de una constante individual
    typedef struct {
        FOXY_CONSTANT_TYPE type;
        union {
            int64_t ival;
            char* sval;
        } as;
    } FoxyConstant;

    // Bloque de Control de Proceso (PCB virtual)
    typedef struct {
        int pid;
        char name[32];
        FOXY_PROCESS_STATE state;
        
        uint8_t* bytecode;     // Cambiado a no-const para liberar/asignar con malloc libremente
        size_t bytecode_size;
        size_t ip;
        
        uint64_t* stack;
        size_t stack_top;
    } FoxyProcess;

    // Estructura global de la Máquina Virtual (FoxyVM con etiqueta)
    typedef struct FoxyVM {
        FoxyProcess** processes;
        size_t process_count;
        size_t process_capacity;
        int current_process_index;
        
        char** loaded_libs;
        size_t loaded_libs_count;
        size_t loaded_libs_capacity;
        
        // --- Constant Pool global de la VM ---
        FoxyConstant* constants;
        size_t constants_count;
        size_t constants_capacity;

        bool running;
    } FoxyVM;

    FoxyVM* f_vm_create(void);
    void f_vm_free(FoxyVM* vm);
    bool f_vm_load_process(FoxyVM* vm, const uint8_t* code, size_t size, const char* name);
    bool f_vm_load_script_to_process(FoxyVM* vm, ASTNode* ast_root, int pid);
    int f_vm_run(FoxyVM* vm);
#endif