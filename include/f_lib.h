// f_lib.h
#ifndef F_LIB_H
    #define F_LIB_H

    #include "f_settings.h"
    #include <stdbool.h>
    #include <dlfcn.h>
    #include "uthash.h"
    #include "f_settings.h"
    #include "f_gc.h"

    typedef struct FoxyState FoxyState;
    typedef struct FoxyModule {
        char name[FOXY_MAX_MODULE_NAME_SIZE];
        FoxyGCHandle *handle;
        bool is_loaded;
        UT_hash_handle hh;
    } FoxyModule;

    void f_lib_load_nativelib(FoxyState *F, const char* path); // modulos de f_include/ y derivados.
    void f_lib_load_userlib(FoxyState *F, const char* path);   // miModulo.foxy
#endif // F_LIB_H