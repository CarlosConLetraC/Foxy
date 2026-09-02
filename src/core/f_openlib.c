#include "f_settings.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "f_openlib.h"
#include "f_vm.h"

typedef void (*FoxyInitModuleFunc)(FoxyVM *vm);

static void *f_vm_resolve_and_dlopen(const char *cpath, const char *lib_name, char *out_resolved_path, size_t max_len) {
    char template_buffer[512];
    const char *start = cpath;
    const char *end;
    
    // Limpiar errores previos de dlopen
    dlerror();

    while (start && *start != '\0') {
        end = strchr(start, ';');
        size_t len = end ? (size_t)(end - start) : strlen(start);

        if (len > 0 && len < sizeof(template_buffer)) {
            strncpy(template_buffer, start, len);
            template_buffer[len] = '\0';

            char *qmark = strchr(template_buffer, '?');
            if (qmark) {
                size_t prefix_len = qmark - template_buffer;
                snprintf(out_resolved_path, max_len, "%.*s%s%s",
                         (int)prefix_len, template_buffer,
                         lib_name,
                         qmark + 1);
            } else {
                snprintf(out_resolved_path, max_len, "%s", template_buffer);
            }

            // Si el archivo existe físicamente en el disco, intentamos dlopen
            if (access(out_resolved_path, F_OK) == 0) {
                void *handle = dlopen(out_resolved_path, RTLD_NOW | RTLD_GLOBAL);
                if (handle) {
                    return handle; // Encontrado y cargado con éxito
                }
                
                // Si el archivo EXISTE pero dlopen falló (ej. símbolo faltante o bad ELF)
                const char *err = dlerror();
                fprintf(stderr, "[Foxy OpenLib Debug] Fallo al cargar '%s': %s\n", 
                        out_resolved_path, err ? err : "Error desconocido");
                return NULL;
            }
        }

        start = end ? end + 1 : NULL;
    }

    return NULL;
}

bool f_vm_load_library(FoxyVM *vm, const char *lib_name) {
    if (!vm || !lib_name) return false;

    // 1. Evitar recargar si ya está cargada
    for (size_t i = 0; i < vm->loaded_libs_count; i++) {
        if (strcmp(vm->loaded_libs[i], lib_name) == 0) return true;
    }

    char resolved_path[512] = {0};
    
    // Si vm->cpath fuera dinámico podrías usarlo aquí, o usar el macro por defecto
    const char *cpath = FOXY_DEFAULT_CPATH;

    void *handle = f_vm_resolve_and_dlopen(cpath, lib_name, resolved_path, sizeof(resolved_path));

    if (!handle) {
        fprintf(stderr, "[Foxy VM Error] No se pudo cargar la librería '%s' (cpath: \"%s\"): %s\n", 
                lib_name, cpath, dlerror());
        return false;
    }

    // 2. Resolver punto de entrada foxy_init_module
    FoxyInitModuleFunc init_module = (FoxyInitModuleFunc)dlsym(handle, "foxy_init_module");
    if (!init_module) {
        fprintf(stderr, "[Foxy VM Error] Símbolo 'foxy_init_module' no encontrado en '%s': %s\n", 
                resolved_path, dlerror());
        dlclose(handle);
        return false;
    }

    // 3. Inicializar módulo
    init_module(vm);

    // 4. Registrar la librería en la VM
    if (vm->loaded_libs_count >= vm->loaded_libs_capacity) {
        size_t new_cap = vm->loaded_libs_capacity == 0 ? 8 : vm->loaded_libs_capacity * 2;
        char **new_libs = (char **)realloc(vm->loaded_libs, sizeof(char *) * new_cap);
        if (new_libs) {
            vm->loaded_libs = new_libs;
            vm->loaded_libs_capacity = new_cap;
        }
    }

    if (vm->loaded_libs_count < vm->loaded_libs_capacity) {
        vm->loaded_libs[vm->loaded_libs_count++] = strdup(lib_name);
    }

    return true;
}