// f_protocol.h
#ifndef F_PROTOCOL_H
    #define F_PROTOCOL_H

    #include <pthread.h>
    #include "uthash.h"
    #include "f_symtable.h"

    typedef struct FoxyProtocol {
        char name[64];           // Clave del protocolo (ej. "pruebaEntorno")
        FoxySymbolTable *symtable;  // Tabla relacional de símbolos compartidos
        pthread_mutex_t lock;    // Mutex para lectura/escritura segura entre threads
        UT_hash_handle hh;
    } FoxyProtocol;

    FoxyProtocol* f_protocol_new(const char *name);
    void f_protocol_free(FoxyProtocol *protocol);
#endif // F_PROTOCOL_H