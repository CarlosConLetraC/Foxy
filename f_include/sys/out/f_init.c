// f_include/sys/out/f_init.c
#include "f_settings.h"
#include "f_vm.h"
#include "f_lib.h"
#include "f_value.h"
#include "f_symtable.h"
#include "f_init.h"
#include <string.h>

FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc);

FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    // 1. Mantenemos el registro en la symtable si lo requiere tu arquitectura relacional
    FoxyLib *current_lib = f_vm_get_current_loading_lib(vm);
    if (current_lib) {
        FoxyValue native_print = {0}; // { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_print };
        native_print.type = FOXY_VAL_FUNCTION;
        native_print.as.native_fn = (void *)f_sys_out_print;

        FoxyValue native_printf = {0}; // { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_printf };
        native_printf.type = FOXY_VAL_FUNCTION;
        native_printf.as.native_fn = (void *)f_sys_out_printf;

        FoxyValue native_println = {0}; // { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_println };
        native_println.type = FOXY_VAL_FUNCTION;
        native_println.as.native_fn = (void *)f_sys_out_println;
        uint32_t module_id = (uint32_t)(vm->process_count + 1);

        if (vm->symtable) {
            f_symtable_insert(vm->symtable, module_id, "print", native_print);
            f_symtable_insert(vm->symtable, module_id, "printf", native_printf);
            f_symtable_insert(vm->symtable, module_id, "println", native_println);
        }
    }

    // 2. REGISTRO PLANO (Optimizado): Inyección directa al motor nativo de la VM
    f_vm_register_native(vm, "print", f_sys_out_print);
    f_vm_register_native(vm, "printf", f_sys_out_printf);
    f_vm_register_native(vm, "println", f_sys_out_println);
}