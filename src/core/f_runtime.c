#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "f_callstack.h"
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

    // 1. Detener/esperar procesos fuera del lock crítico si es posible
    // o asegurar el join antes de mutar la estructura
    pthread_mutex_lock(&rt->global_lock);

    // Liberar procesos
    FoxyProcess *curr_proc, *tmp_proc;
    HASH_ITER(hh, rt->processes, curr_proc, tmp_proc) {
        HASH_DEL(rt->processes, curr_proc);
        
        // Esperar a que el hilo termine si sigue activo
        if (curr_proc->state == FOXY_PROCESS_RUNNING) {
            pthread_mutex_unlock(&rt->global_lock);
            pthread_join(curr_proc->thread_id, NULL);
            pthread_mutex_lock(&rt->global_lock);
        }

        // Liberar librerías locales
        FoxyLib *curr_llib, *tmp_llib;
        HASH_ITER(hh, curr_proc->locallibs, curr_llib, tmp_llib) {
            HASH_DEL(curr_proc->locallibs, curr_llib);
            // if (curr_llib->name) free(curr_llib->name);
            free(curr_llib);
        }

        // Liberar callstack y buffers del proceso
        if (curr_proc->call_stack.frames) free(curr_proc->call_stack.frames);
        if (curr_proc->stack) free(curr_proc->stack);
        if (curr_proc->locals) free(curr_proc->locals);

        free(curr_proc);
    }

    // 2. Liberar protocolos compartidos
    FoxyProtocol *curr_prot, *tmp_prot;
    HASH_ITER(hh, rt->protocols, curr_prot, tmp_prot) {
        HASH_DEL(rt->protocols, curr_prot);
        if (curr_prot->symtable) {
            f_symtable_free(curr_prot->symtable);
        }
        pthread_mutex_destroy(&curr_prot->lock);
        free(curr_prot);
    }

    // 3. Liberar librerías globales del sistema
    FoxyLib *curr_glib, *tmp_glib;
    HASH_ITER(hh, rt->loadedlibs, curr_glib, tmp_glib) {
        HASH_DEL(rt->loadedlibs, curr_glib);
    #ifdef FOXY_ENABLE_DYNAMIC_LOADING
        if (curr_glib->handle) {
            dlclose(curr_glib->handle);
        }
    #endif
        free(curr_glib);
    }

    // Unlocking antes de destruir el mutex
    pthread_mutex_unlock(&rt->global_lock);
    pthread_mutex_destroy(&rt->global_lock);

    free(rt);
}
// --- Gestión de Procesos (processes) ---

FoxyProcess* f_runtime_process_create(FoxyRuntime *rt, const char *pname, FoxyFunction *main_func, FoxyProtocol *protocol) {
    if (!rt || !pname || !main_func) return NULL;

    pthread_mutex_lock(&rt->global_lock);

    // Verificar si ya existe un proceso registrado por su nombre
    FoxyProcess *existing = NULL;
    HASH_FIND_STR(rt->processes, pname, existing);
    if (existing != NULL) {
        pthread_mutex_unlock(&rt->global_lock);
        return NULL; // Nombre de proceso duplicado
    }

    FoxyProcess *proc = (FoxyProcess*)calloc(1, sizeof(FoxyProcess));
    if (!proc) {
        pthread_mutex_unlock(&rt->global_lock);
        return NULL;
    }

    strncpy(proc->pname, pname, sizeof(proc->pname) - 1);
    proc->pname[sizeof(proc->pname) - 1] = '\0';
    
    proc->state = FOXY_PROCESS_READY;

    // Entornos aislados y dependencias
    proc->locallibs = NULL;
    proc->protocol = protocol;

    // Inicializar el CallStack del proceso e insertar la función principal
    f_callstack_init(&proc->call_stack);
    
    if (!f_callstack_push(&proc->call_stack, main_func, 0, 0)) {
        free(proc);
        pthread_mutex_unlock(&rt->global_lock);
        return NULL;
    }

    // Registrar en el hashmap global usando el nombre como clave
    HASH_ADD_STR(rt->processes, pname, proc);

    pthread_mutex_unlock(&rt->global_lock);
    return proc;
}

FoxyProcess* f_runtime_process_get(FoxyRuntime *rt, FoxyProcess *proc_ptr) {
    if (!rt || !proc_ptr) return NULL;
    pthread_mutex_lock(&rt->global_lock);

    FoxyProcess *proc = NULL;
    HASH_FIND_PTR(rt->processes, &proc_ptr, proc);

    pthread_mutex_unlock(&rt->global_lock);
    return proc;
}

bool f_runtime_process_start(FoxyProcess *process) {
    if (!process || process->state != FOXY_PROCESS_READY) return false;

    // Lanza el proceso en un hilo nativo del SO
    if (pthread_create(&process->thread_id, NULL, f_process_worker, process) != 0) {
        process->state = FOXY_PROCESS_DEAD;
        return false;
    }

    return true;
}

// --- Gestión de Protocolos / SharedEnv (protocols) ---

FoxyProtocol* f_runtime_get_or_create(FoxyRuntime *rt, const char *name) {
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