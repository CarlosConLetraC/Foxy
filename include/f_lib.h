// f_lib.h
#ifndef F_LIB_H
    #define F_LIB_H

    #include "uthash.h"
    #include "f_settings.h"

    typedef struct FoxyLib {
        const void *ptr_id;     // Clave por puntero/nombre de la librería
        void *handle;           // Manejador si es .so (dlopen)
        char path[FOXY_MAX_MODULE_NAME_SIZE]; // Ruta del módulo requerida por f_init.c
        UT_hash_handle hh;
    } FoxyLib;

    FoxyLib* f_lib_new(const void *ptr_id, void *handle);
    void f_lib_free(FoxyLib *lib);
#endif // F_LIB_H