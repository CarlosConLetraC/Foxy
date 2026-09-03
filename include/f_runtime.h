#ifndef F_RUNTIME_H
    #define F_RUNTIME_H

    #include <stdbool.h>
    #include <stdint.h>
    #include <pthread.h>
    #include "uthash.h"

    typedef struct FoxyProcess FoxyProcess;
    typedef struct FoxyProtocol FoxyProtocol;
    typedef struct FoxyLib FoxyLib;

    typedef struct FoxyRuntime {
        FoxyLib *loadedlibs;
        FoxyProtocol *protocols;
        FoxyProcess *processes;
        pthread_mutex_t global_lock;
    } FoxyRuntime;

    FoxyRuntime* f_runtime_new(void);
    void f_runtime_free(FoxyRuntime *rt);

    FoxyProcess* f_process_create(FoxyRuntime *rt, const char *pname, const uint8_t *bytecode, FoxyProtocol *protocol);
    bool f_process_start(FoxyProcess *process);
    FoxyProtocol* f_protocol_get_or_create(FoxyRuntime *rt, const char *name);
#endif // F_RUNTIME_H