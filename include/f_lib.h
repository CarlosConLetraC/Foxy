// f_lib.h
#ifndef F_LIB_H
    #define F_LIB_H

    #include "uthash.h"

    typedef struct FoxyLib {
        const void *ptr_id;     // Clave por puntero/nombre de la librería
        void *handle;           // Manejador si es .so (dlopen)
        UT_hash_handle hh;
    } FoxyLib;

    FoxyLib* f_lib_new(const void *ptr_id, void *handle);
    void f_lib_free(FoxyLib *lib);
#endif // F_LIB_H