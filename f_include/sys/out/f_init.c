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

    FoxyLib *current_lib = f_vm_get_current_loading_lib(vm);
    if (!current_lib) return;

    FoxyValue print_val   = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_print };
    FoxyValue printf_val  = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_printf };
    FoxyValue println_val = { .type = FOXY_VAL_FUNCTION, .as.native_fn = (void *)f_sys_out_println };

    uint32_t module_id = (uint32_t)(vm->process_count + 1);

    // Registrar las funciones nativas directamente en la tabla relacional unificada de símbolos (`vm->symtable`)
    if (vm->symtable) {
        f_symtable_insert(vm->symtable, module_id, "print", print_val);
        f_symtable_insert(vm->symtable, module_id, "printf", printf_val);
        f_symtable_insert(vm->symtable, module_id, "println", println_val);
    }
}