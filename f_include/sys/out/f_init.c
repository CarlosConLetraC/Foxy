// f_include/sys/out/f_init.c
#include "f_vm.h"
#include "f_init.h"

FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc);
FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc);

// Cambiado de f_sys_out_init a foxy_init_module
FOXY_EXPORT void foxy_init_module(FoxyVM *vm) {
    if (!vm) return;

    // Registrar cada función nativa bajo su nombre correspondiente
    f_vm_register_native(vm, "print", f_sys_out_print);
    f_vm_register_native(vm, "printf", f_sys_out_printf);
    f_vm_register_native(vm, "println", f_sys_out_println);
}