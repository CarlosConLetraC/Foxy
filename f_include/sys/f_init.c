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

    // Deriva dinámicamente el nombre del módulo (ej: "sys" o el submódulo correspondiente)[cite: 1]
    const char *last_slash = strrchr(current_lib->path, '/');
    const char *module_name = last_slash ? last_slash + 1 : current_lib->path;

    // Obtiene la instancia del objeto contenedor[cite: 1]
    FoxyObject *out_instance = f_out_module_init();
    if (out_instance) {
        FoxyValue mod_val = {0};
        mod_val.type = FOXY_VAL_OBJECT;
        mod_val.as.ptr = out_instance;

        uint32_t module_id = (uint32_t)(vm->process_count + 1);

        // Inserta el módulo completo como un objeto en la symtable para acceso por puntos (ej: sys.out)[cite: 1]
        if (vm->symtable) f_symtable_insert(vm->symtable, module_id, module_name, mod_val);
    }
}