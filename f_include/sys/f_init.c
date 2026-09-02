#include "f_init.h"
#include "f_dict.h"
#include "f_object.h"

// Declaración del inicializador del submódulo interno 'out' 
// (que puede estar definido en out/f_init.c)
extern FoxyObject* f_out_module_init(void); 

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    // 1. Crear el diccionario raíz del espacio de nombres 'sys' usando uthash
    FoxyDict *sys_dict = f_dict_new();
    if (!sys_dict) return;

    // 2. Inicializar y obtener la instancia del objeto 'out'
    FoxyObject *out_instance = f_out_module_init();
    if (out_instance) {
        // 3. Empaquetarlo en un FoxyValue e insertarlo en el diccionario 'sys' con la clave "out"
        FoxyValue out_val = {
            .type = FOXY_VAL_OBJECT,
            .as.object = out_instance
        };

        f_dict_set(sys_dict, "out", out_val);
    }

    // 4. Registrar el diccionario 'sys' globalmente en la VM (ejemplo conceptual)
    // f_vm_register_global(vm, "sys", (FoxyValue){.type = FOXY_VAL_DICT, .as.dict = sys_dict});
}