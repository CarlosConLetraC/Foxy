#include "f_settings.h"
#include "f_vm.h"
#include "f_value.h"
#include "f_object.h"
#include "f_lib.h"
#include "f_symtable.h"
#include "f_init.h"
#include <string.h>

// Declaración externa del inicializador de 'out'
extern FoxyObject* f_out_module_init(void); 

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    FoxyLib *current_lib = f_vm_get_current_loading_lib(vm);
    if (!current_lib || !current_lib->path) return;

    // Deriva el nombre del módulo dinámicamente desde la ruta de la librería
    const char *last_slash = strrchr(current_lib->path, '/');
    const char *module_name = last_slash ? last_slash + 1 : current_lib->path;

    // Instancia del objeto contenedor de submódulo
    FoxyObject *out_instance = f_out_module_init();
    if (out_instance) {
        FoxyValue mod_val = { .type = FOXY_VAL_OBJECT, .as.obj = out_instance };

        uint32_t module_id = (uint32_t)F_SYS_OUT_GET_CURRENT_PROCESS(vm); // (uint32_t)vm->current_process_index;

        if (vm->symtable) {
            f_symtable_insert(vm->symtable, module_id, module_name, mod_val);
        }
    }
}