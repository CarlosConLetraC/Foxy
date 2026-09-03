// f_include/sys/f_init.c
#include "f_init.h"
#include "f_dict.h"
#include "f_object.h"
#include "f_vm.h"

// Declaración del inicializador del submódulo interno 'out' 
extern FoxyObject* f_out_module_init(void); 

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    // 1. Crear el diccionario raíz del espacio de nombres 'sys'
    FoxyDict *sys_dict = f_dict_new();
    if (!sys_dict) return;

    // 2. Inicializar y obtener la instancia del objeto 'out'
    FoxyObject *out_instance = f_out_module_init();
    if (out_instance) {
        FoxyValue out_val = {
            .type = FOXY_VAL_OBJECT,
            .as.object = out_instance
        };
        f_dict_set(sys_dict, "out", out_val);
    }

    // 3. Registrar el diccionario 'sys' como un valor global accesible en la VM
    FoxyValue sys_val = {
        .type = FOXY_VAL_DICT,
        .as.dict = sys_dict
    };
    
    // Si tienes una función para registrar globales en la VM, utilízala aquí:
    // f_vm_register_global(vm, "sys", sys_val);
}