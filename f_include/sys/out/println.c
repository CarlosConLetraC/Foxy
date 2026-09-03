#include "f_value.h"
#include "f_vm.h"
#include "f_init.h"
#include "f_utils.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    if (!vm || vm->process_count == 0) {
        f_utils_syswrite(1, "\n", 1);
        return;
    }

    FoxyProcess *p = vm->processes[vm->current_process_index];

    // Leer directamente de la pila sin desapilar
    for (int i = 0; i < argc; i++) {
        // Obtenemos el argumento desde la pila según su índice de izquierda a derecha
        size_t dist = argc - 1 - i;
        FoxyValue arg = f_vm_peek(p, dist);

        f_utils_print_constant_dynamic(arg);

        if (i < argc - 1) {
            f_utils_syswrite(1, " ", 1);
        }
    }

    f_utils_syswrite(1, "\n", 1);
}