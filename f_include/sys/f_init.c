// f_include/sys/f_init.c
#include "f_settings.h" // Asegura el uso de FOXY_EXPORT
#include "f_vm.h"
#include "f_value.h"
#include "f_lib.h"
#include "f_symtable.h"
#include "f_init.h"
#include <string.h>

// Declaración del inicializador del submódulo interno
extern FoxyObject* f_out_module_init(void); 

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    FoxyLib *current_lib = f_vm_get_current_loading_lib(vm);
    if (!current_lib || !current_lib->path) return;

    // 1. Derivar dinámicamente el nombre del módulo usando su path (estilo init.lua)
    // Ej: "sys/out" -> extrae "out" como el nombre del submódulo actual
    const char *last_slash = strrchr(current_lib->path, '/');
    const char *module_name = last_slash ? last_slash + 1 : current_lib->path;

    // 2. Inicializar y obtener la instancia del objeto correspondiente
    FoxyObject *out_instance = f_out_module_init();
    if (out_instance) {
        FoxyValue mod_val = {
            .type = FOXY_VAL_OBJECT,
            .as.ptr = out_instance
        };
        
        uint32_t module_id = (uint32_t)(vm->process_count + 1);

        // 3 y 4. Registrar el submódulo directamente en la tabla relacional unificada de símbolos (`f_symtable`)
        if (vm->symtable) {
            f_symtable_insert(vm->symtable, module_id, module_name, mod_val);
        }
    }
}