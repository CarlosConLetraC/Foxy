// src/core/f_lib.c
#include "f_lib.h"
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

FoxyLib* f_lib_new(const void *ptr_id, void *handle) {
    if (!ptr_id) return NULL;

    FoxyLib *lib = (FoxyLib *)malloc(sizeof(FoxyLib));
    if (!lib) {
        fprintf(stderr, "[Foxy Error] Out of memory allocating FoxyLib\n");
        return NULL;
    }

    lib->ptr_id = ptr_id;
    lib->handle = handle;

    // Inicializar el manejador hash de uthash en cero
    lib->hh.tbl = NULL;
    lib->hh.next = NULL;
    lib->hh.prev = NULL;

    return lib;
}

void f_lib_free(FoxyLib *lib) {
    if (!lib) return;

    // 1. Cerrar el manejador de la librería dinámica si fue abierta con dlopen
    if (lib->handle) {
        dlclose(lib->handle);
        lib->handle = NULL;
    }

    // 2. Liberar la estructura principal de la librería
    free(lib);
}