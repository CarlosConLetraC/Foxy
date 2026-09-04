#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!proc) return;

    // Imprimir argumentos si existen
    if (argc >= 1) {
        for (int i = 0; i < argc; i++) {
            FoxyValue val = f_vm_peek(proc, argc - 1 - i);
            f_utils_print_constant_dynamic(*(FoxyConstant*)&val, -1);
        }
    }

    // Salto de línea final
    f_utils_syswrite(1, "\n", 1);
}