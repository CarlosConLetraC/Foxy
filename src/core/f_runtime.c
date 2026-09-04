#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "f_runtime.h"
#include "f_symtable.h"
#include "f_utils.h"
#include "f_process.h"  
#include "f_protocol.h"
#include "f_lib.h"
#include "f_vm.h"

// --- Inicialización y Destrucción del Runtime Global ---

FoxyRuntime* f_runtime_new(void) {
    FoxyRuntime *rt = (FoxyRuntime*)malloc(sizeof(FoxyRuntime));
    if (!rt) return NULL;

    // Inicializar las tablas hash principales del FOXY_HASH_MAP a NULL (requerido por uthash)
    rt->loadedlibs = NULL;
    rt->protocols = NULL;
    rt->processes = NULL;

    if (pthread_mutex_init(&rt->global_lock, NULL) != 0) {
        free(rt);
        return NULL;
    }

    return rt;
}

void f_runtime_free(FoxyRuntime *rt) {
    if (!rt) return;

    pthread_mutex_lock(&rt->global_lock);

    // 1. Liberar procesos (processes)
    FoxyProcess *curr_proc, *tmp_proc;
    HASH_ITER(hh, rt->processes, curr_proc, tmp_proc) {
        HASH_DEL(rt->processes, curr_proc);
        
        // Esperar a que el hilo termine si aún sigue activo
        if (curr_proc->state == FOXY_PROCESS_RUNNING) {
            pthread_join(curr_proc->thread_id, NULL);
        }

        // Liberar librerías locales
        FoxyLib *curr_llib, *tmp_llib;
        HASH_ITER(hh, curr_proc->locallibs, curr_llib, tmp_llib) {
            HASH_DEL(curr_proc->locallibs, curr_llib);
            free(curr_llib);
        }

        free(curr_proc);
    }

    // 2. Liberar protocolos compartidos (protocols)
    FoxyProtocol *curr_prot, *tmp_prot;
    HASH_ITER(hh, rt->protocols, curr_prot, tmp_prot) {
        HASH_DEL(rt->protocols, curr_prot);
        if (curr_prot->symtable) {
            f_symtable_free(curr_prot->symtable);
        }
        pthread_mutex_destroy(&curr_prot->lock);
        free(curr_prot);
    }

    // 3. Liberar librerías globales del sistema (loadedlibs)
    FoxyLib *curr_glib, *tmp_glib;
    HASH_ITER(hh, rt->loadedlibs, curr_glib, tmp_glib) {
        HASH_DEL(rt->loadedlibs, curr_glib);
        if (curr_glib->handle) {
            // dlclose(curr_glib->handle); // Descomentar si se usa dlfcn.h
        }
        free(curr_glib);
    }

    pthread_mutex_unlock(&rt->global_lock);
    pthread_mutex_destroy(&rt->global_lock);

    free(rt);
}

// --- Gestión de Procesos (processes) ---

FoxyProcess* f_process_create(FoxyRuntime *rt, const char *pname, const uint8_t *bytecode, FoxyProtocol *protocol) {
    if (!rt || !pname) return NULL;

    pthread_mutex_lock(&rt->global_lock);

    // Verificar si ya existe un proceso con el mismo nombre
    FoxyProcess *existing = NULL;
    HASH_FIND_STR(rt->processes, pname, existing);
    if (existing != NULL) {
        pthread_mutex_unlock(&rt->global_lock);
        return NULL; // Nombre de proceso duplicado
    }

    FoxyProcess *proc = (FoxyProcess*)malloc(sizeof(FoxyProcess));
    if (!proc) {
        pthread_mutex_unlock(&rt->global_lock);
        return NULL;
    }

    strncpy(proc->pname, pname, sizeof(proc->pname) - 1);
    proc->pname[sizeof(proc->pname) - 1] = '\0';
    
    static int pid_counter = 1;
    proc->pid = pid_counter++;
    proc->state = FOXY_PROCESS_READY;

    // Entornos aislados y dependencias
    proc->locallibs = NULL;
    proc->protocol = protocol; // Enlace al SharedEnv (puede ser NULL)

    // Bytecode
    proc->bytecode = bytecode;
    proc->ip = 0;
    proc->stack_top = 0;

    // Registrar en el hashmap global
    HASH_ADD_STR(rt->processes, pname, proc);

    pthread_mutex_unlock(&rt->global_lock);
    return proc;
}

// --- Worker de Hilo POSIX ---

static void* f_vm_process_worker(void *arg) {
    FoxyProcess *proc = (FoxyProcess*)arg;
    proc->state = FOXY_PROCESS_RUNNING;

    // Loop principal del intérprete para este proceso
    while (proc->state == FOXY_PROCESS_RUNNING && proc->bytecode != NULL) {
        uint8_t opcode = proc->bytecode[proc->ip++];

        // Simulación: detener al llegar a un opcode de fin (0x00 / OP_HALT)
        if (opcode == 0x00) {
            break;
        }
    }

    proc->state = FOXY_PROCESS_DEAD;
    return NULL;
}

bool f_process_start(FoxyProcess *process) {
    if (!process || process->state != FOXY_PROCESS_READY) return false;

    // Lanza el proceso en un hilo nativo del SO
    if (pthread_create(&process->thread_id, NULL, f_vm_process_worker, process) != 0) {
        process->state = FOXY_PROCESS_DEAD;
        return false;
    }

    return true;
}

// --- Gestión de Protocolos / SharedEnv (protocols) ---

FoxyProtocol* f_protocol_get_or_create(FoxyRuntime *rt, const char *name) {
    if (!rt || !name) return NULL;

    pthread_mutex_lock(&rt->global_lock);

    FoxyProtocol *prot = NULL;
    HASH_FIND_STR(rt->protocols, name, prot);

    if (!prot) {
        prot = (FoxyProtocol*)malloc(sizeof(FoxyProtocol));
        if (!prot) {
            pthread_mutex_unlock(&rt->global_lock);
            return NULL;
        }

        strncpy(prot->name, name, sizeof(prot->name) - 1);
        prot->name[sizeof(prot->name) - 1] = '\0';
        
        // Inicializar la tabla relacional de símbolos compartidos para el protocolo
        prot->symtable = f_symtable_new();
        pthread_mutex_init(&prot->lock, NULL);

        HASH_ADD_STR(rt->protocols, name, prot);
    }

    pthread_mutex_unlock(&rt->global_lock);
    return prot;
}