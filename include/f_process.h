#ifndef F_PROCESS_H
    #define F_PROCESS_H

    #include "f_settings.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <pthread.h>
    #include "uthash.h"
    #include "f_value.h"
    #include "f_lib.h"     
    #include "f_runtime.h"
    #include "f_callstack.h"

    typedef struct FoxyRuntime FoxyRuntime;
    typedef struct FoxyProtocol FoxyProtocol;
    typedef struct FoxyVM FoxyVM;

    #define FOXY_PROCESS_STATE_LIST(F) \
        F(FOXY_PROCESS_READY,   "READY")   \
        F(FOXY_PROCESS_RUNNING, "RUNNING") \
        F(FOXY_PROCESS_WAITING, "WAITING") \
        F(FOXY_PROCESS_DEAD,    "DEAD")

    #define F(name, str) name,
    typedef enum __attribute__((__packed__)){
        FOXY_PROCESS_STATE_LIST(F)
        FOXY_PROCESS_STATE_COUNT
    } FoxyProcessState;
    #undef F

    extern const char* FOXY_PROCESS_STATE_NAMES[];

    typedef struct FoxyProcess {
        FoxyProcessState state;
        char name[FOXY_MAX_IDENTIFIER_LEN];
        char pname[FOXY_MAX_IDENTIFIER_LEN];
        pthread_t thread_id;

        // VM Execution State
        FoxyVM *vm;
        FoxyCallStack call_stack;

        // Evaluation Stack
        FoxyValue *stack;
        size_t stack_top;
        size_t stack_capacity;

        // Local Variables
        FoxyValue *locals;
        size_t locals_count;
        size_t locals_capacity;

        // Process Isolation / Environments
        FoxyLib *locallibs;    
        FoxyProtocol *protocol;

        int running;
        UT_hash_handle hh;
        FoxyFunction *main_func;
    } FoxyProcess;

    const char* f_process_state_to_string(FoxyProcessState state);
    FoxyProcess* f_process_create(FoxyRuntime *rt, const char *pname, FoxyFunction *main_func, FoxyProtocol *protocol);
    bool f_process_start(FoxyProcess *process);
    void f_process_free(FoxyProcess *process);
    void* f_process_worker(void *arg);
    FoxyValue f_process_pop(FoxyProcess *p);
#endif // F_PROCESS_H