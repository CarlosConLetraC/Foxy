#include "f_openlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef void (*FoxyInitModuleFunc)(FoxyVM *vm);

bool f_vm_load_library(FoxyVM *vm, const char *lib_name) {
    fprintf(stderr, "[Foxy Loader Trace] Entrando a f_vm_load_library. vm=%p, lib_name=%p\n", (void*)vm, (void*)lib_name);
    
    if (!vm || !lib_name) {
        fprintf(stderr, "[Foxy Loader Error] Parámetro nulo recibido.\n");
        return false;
    }

    fprintf(stderr, "[Foxy Loader Trace] Valor de lib_name: '%s'\n", lib_name);

    char path_buf[512];

    // Mapeo de rutas más robusto y flexible
    if (strchr(lib_name, '/') != NULL) {
        // Si ya trae subdirectorio (ej. "sys/out"), probamos primero en f_include/ y luego con ruta directa
        snprintf(path_buf, sizeof(path_buf), "f_include/%s.so", lib_name);
    } else {
        snprintf(path_buf, sizeof(path_buf), "./f_include/%s.so", lib_name);
    }

    fprintf(stderr, "[Foxy Loader Trace] Ruta calculada 1: %s\n", path_buf);

    void *handle = dlopen(path_buf, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "[Foxy Loader Trace] Falló dlopen en '%s'. Error: %s\n", path_buf, dlerror());
        
        // Intento secundario usando FOXY_DEFAULT_HOME si está definido
        #ifdef FOXY_DEFAULT_HOME
        snprintf(path_buf, sizeof(path_buf), "%s/lib/%s.so", FOXY_DEFAULT_HOME, lib_name);
        fprintf(stderr, "[Foxy Loader Trace] Intentando ruta alternativa (HOME): %s\n", path_buf);
        handle = dlopen(path_buf, RTLD_NOW | RTLD_GLOBAL);
        #endif

        if (!handle) {
            snprintf(path_buf, sizeof(path_buf), "%s.so", lib_name);
            fprintf(stderr, "[Foxy Loader Trace] Intentando ruta directa: %s\n", path_buf);
            handle = dlopen(path_buf, RTLD_NOW | RTLD_GLOBAL);
        }

        if (!handle) {
            fprintf(stderr, "[Foxy Dynamic Loader Error] Imposible cargar '%s'. Último error: %s\n", lib_name, dlerror());
            return false;
        }
    }

    fprintf(stderr, "[Foxy Loader Trace] Librería abierta con éxito. Buscando símbolo 'f_module_init'...\n");
    dlerror();

    // Buscar la función de inicialización global del módulo con el nuevo nombre
    FoxyInitModuleFunc init_module = (FoxyInitModuleFunc)dlsym(handle, "f_module_init");
    char *error = dlerror();
    
    if (error != NULL || !init_module) {
        fprintf(stderr, "[Foxy Dynamic Loader Error] Símbolo 'f_module_init' no encontrado en '%s': %s\n", lib_name, error ? error : "Desconocido");
        dlclose(handle);
        return false;
    }

    fprintf(stderr, "[Foxy Loader Trace] Símbolo encontrado. Ejecutando f_module_init(vm)...\n");

    // Ejecutar la inicialización para registrar las funciones nativas en la VM
    init_module(vm);
    fprintf(stderr, "[Foxy Loader Trace] Módulo inicializado correctamente.\n");
    return true;
}