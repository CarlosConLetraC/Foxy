#include "f_settings.h"
#include "f_vm.h"
#include "f_lib.h"
#include "f_value.h"
#include "f_object.h"
#include "f_symtable.h"
#include "f_init.h"
#include <string.h>

FOXY_EXPORT FoxyObject* f_out_module_init(void) {
    // Se pasa NULL porque es un objeto/módulo contenedor sin FoxyClass formal
    FoxyObject *out_obj = f_object_new(NULL);
    if (!out_obj) return NULL;

    FoxyValue fn_print = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_print };
    FoxyValue fn_printf = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_printf };
    FoxyValue fn_println = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_println };

    f_object_set_field(out_obj, "print", fn_print);
    f_object_set_field(out_obj, "printf", fn_printf);
    f_object_set_field(out_obj, "println", fn_println);

    return out_obj;
}

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    FoxyLib *current_lib = f_vm_get_current_loading_lib(vm);
    if (current_lib) {
        FoxyValue native_print = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_print };
        FoxyValue native_printf = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_printf };
        FoxyValue native_println = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_println };

        FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
        uint32_t module_id = (uint32_t)(uintptr_t)proc;

        if (vm->symtable) {
            f_symtable_insert(vm->symtable, module_id, "print", native_print);
            f_symtable_insert(vm->symtable, module_id, "printf", native_printf);
            f_symtable_insert(vm->symtable, module_id, "println", native_println);
        }
    }

    // Registro plano optimizado en el Hash de la VM de uthash
    f_vm_register_native(vm, "print", (FoxyNativeMethod)f_sys_out_print);
    f_vm_register_native(vm, "printf", (FoxyNativeMethod)f_sys_out_printf);
    f_vm_register_native(vm, "println", (FoxyNativeMethod)f_sys_out_println);
}