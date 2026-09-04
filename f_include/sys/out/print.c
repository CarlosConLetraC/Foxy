#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!proc || argc < 1) return;

    // El primer argumento está en la pila del proceso
    // Dependiendo del orden de tu pila, f_vm_peek te permite leerlo:
    FoxyValue val = f_vm_peek(proc, argc - 1); // o la distancia correspondiente
    f_utils_print_constant_dynamic(*(FoxyConstant*)&val);
}