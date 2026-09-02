#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "f_utils.h"

static const char* f_get_home(void) {
    const char* env_home = getenv("FOXY_HOME");
    if (env_home && env_home[0] != '\0') return env_home;

    #ifdef FOXY_DEFAULT_HOME
        return FOXY_DEFAULT_HOME;
    #else
        return "/opt/foxy-lang";
    #endif
}

void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name) {
    char real_path[1024];
    const char* home = f_get_home();

    // 1. Intentar ruta principal bajo FOXY_HOME/lib/
    snprintf(real_path, sizeof(real_path), "%s/lib/%s.so", home, lib_name);
    void* handle = dlopen(real_path, RTLD_NOW | RTLD_LOCAL);

    // 2. Fallback a ruta local de desarrollo (f_include/) si no se halla en /opt/
    if (!handle) {
        snprintf(real_path, sizeof(real_path), "f_include/%s.so", lib_name);
        handle = dlopen(real_path, RTLD_NOW | RTLD_LOCAL);
    }

    if (!handle) {
        fprintf(stderr, "[Foxy Loader Error] No se pudo cargar la librería '%s': %s\n", lib_name, dlerror());
        return;
    }

    // 3. Buscar punto de entrada y registrar
    typedef int (*foxy_init_fn)(FoxyVM* vm);
    foxy_init_fn init_module = (foxy_init_fn)dlsym(handle, "foxy_module_init");

    if (init_module) {
        init_module(vm);
        printf("[Foxy VM] Módulo nativo cargado: %s\n", real_path);
    } else {
        fprintf(stderr, "[Foxy Loader Error] El módulo no exporta 'foxy_module_init'\n");
    }
}

char* f_utils_read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", filepath);
        return NULL;
    }

    // Ir al final del archivo para medir su tamaño
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Reservar memoria para el contenido + terminador nulo '\0'
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memoria insuficiente para leer '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    // Leer el archivo completo al búfer
    size_t read_bytes = fread(buffer, 1, length, file);
    if (read_bytes != (size_t)length) {
        fprintf(stderr, "Error: No se pudo leer el archivo completo.\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0'; // Asegurar terminación de cadena en C
    fclose(file);
    return buffer;
}
