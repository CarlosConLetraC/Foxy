#ifndef F_PROCESS_H
    #define F_PROCESS_H
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <pthread.h>
    #include "uthash.h"
    #include "f_value.h"
    #include "f_lib.h"     // <--- 1. Incluir el header de la librería

    typedef struct FoxyRuntime FoxyRuntime;
    typedef struct FoxyProtocol FoxyProtocol;
    typedef struct FoxyScope FoxyScope;
    typedef struct FoxyVM FoxyVM;

    #define FOXY_PROCESS_STATE_LIST(F) \
        F(FOXY_PROCESS_READY,   "READY")   \
        F(FOXY_PROCESS_RUNNING, "RUNNING") \
        F(FOXY_PROCESS_WAITING, "WAITING") \
        F(FOXY_PROCESS_DEAD,    "DEAD")

    // 1. Generar el enum automáticamente y el contador total
    #define F(name, str) name,
    typedef enum __attribute__((__packed__)){
        FOXY_PROCESS_STATE_LIST(F)
        FOXY_PROCESS_STATE_COUNT
    } FoxyProcessState;
    #undef F

    typedef struct FoxyProcess {
        uint32_t pid;
        FoxyProcessState state;
        char name[256];
        char pname[256];
        pthread_t thread_id;

        // VM Execution State
        FoxyVM *vm;
        const uint8_t *bytecode;
        size_t bytecode_size;
        size_t ip;

        // Evaluation Stack
        FoxyValue *stack;
        size_t stack_top;      // <--- 2. Asegurarte de usar stack_top
        size_t stack_capacity;

        // Local Variables
        FoxyValue *locals;
        size_t locals_count;
        size_t locals_capacity;

        // Process Isolation / Environments
        FoxyScope *local_scope;
        FoxyLib *locallibs;    // <--- 3. Cambiado de void* a FoxyLib* (con esto uthash compilará perfecto)
        FoxyProtocol *protocol;

        int running;
        UT_hash_handle hh;
    } FoxyProcess;

    const char* f_process_state_to_string(FoxyProcessState state);
    FoxyProcess* f_process_create(FoxyRuntime *rt, const char *pname, const uint8_t *bytecode, FoxyProtocol *protocol);
    bool f_process_start(FoxyProcess *process);
    void f_process_free(FoxyProcess *process);
    FoxyValue f_process_pop(FoxyProcess *p);
#endif // F_PROCESS_H