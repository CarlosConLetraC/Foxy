#include "f_settings.h"
#include <stdlib.h>
#include <string.h>
#include "f_process.h"
#include "f_vm.h"

// Generación automática del arreglo de strings usando la misma X-Macro
const char* FOXY_PROCESS_STATE_NAMES[] = {
    #define F(state, str) str,
    FOXY_PROCESS_STATE_LIST(F)
    #undef F
};

const char* f_process_state_to_string(FoxyProcessState state) {
    if (state < FOXY_PROCESS_STATE_COUNT)
        return FOXY_PROCESS_STATE_NAMES[state];
    return "UNKNOWN_STATE";
}

FoxyProcess* f_process_create(FoxyRuntime *rt, const char *pname, FoxyFunction *main_func, FoxyProtocol *protocol) {
    (void)rt; // por el momento. . .
    FoxyProcess *proc = malloc(sizeof(FoxyProcess));
    if (!proc) return NULL;

    proc->vm = NULL;
    proc->state = FOXY_PROCESS_READY;
    proc->running = 0;
    proc->protocol = protocol;
    proc->locallibs = NULL;

    // Copiar el nombre del proceso de forma segura
    if (pname) {
        strncpy(proc->pname, pname, FOXY_MAX_IDENTIFIER_LEN - 1);
        proc->pname[FOXY_MAX_IDENTIFIER_LEN - 1] = '\0';
    } else {
        strcpy(proc->pname, "main_proc");
    }

    // Inicializar la pila de llamadas
    f_callstack_init(&proc->call_stack);

    // Si se proporciona una función principal, empujarla como el marco inicial (ip = 0, stack_base = 0)
    if (main_func) {
        if (!f_callstack_push(&proc->call_stack, main_func, 0, 0)) {
            free(proc);
            return NULL;
        }
    }

    // Inicializar el stack de evaluación
    proc->stack_capacity = FOXY_MAX_STACK_CAPACITY;
    proc->stack_top = 0;
    proc->stack = malloc(sizeof(FoxyValue) * proc->stack_capacity);

    // Inicializar el arreglo de variables locales
    proc->locals_capacity = FOXY_MAX_LOCALS;
    proc->locals_count = 0;
    proc->locals = malloc(sizeof(FoxyValue) * proc->locals_capacity);

    return proc;
}

void f_process_free(FoxyProcess *proc) {
    if (!proc) return;

    // Solo liberar el stack / registros locales, NO las funciones cargadas desde vm->constants
    if (proc->stack) {
        // Liberar stack si corresponde
        free(proc->stack);
        proc->stack = NULL;
    }

    free(proc);
}

FoxyValue f_process_pop(FoxyProcess *p) {
    if (!p || p->stack_top == 0) {
        return (FoxyValue){ .type = FOXY_VAL_NULL };
    }
    return p->stack[--p->stack_top];
}

void* f_process_worker(void *arg) {
    FoxyProcess *proc = (FoxyProcess *)arg;
    if (!proc || !proc->vm) return NULL;

    f_vm_execute_process(proc->vm, proc);
    proc->state = FOXY_PROCESS_DEAD;
    return NULL;
}

bool f_process_start(FoxyProcess *process) {
    if (!process) return false;
    process->running = 1;
    
    if (pthread_create(&process->thread_id, NULL, f_process_worker, process) != 0) {
        process->running = 0;
        return false;
    }
    return true;
}