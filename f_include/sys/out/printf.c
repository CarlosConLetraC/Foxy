#include "f_value.h"
#include "f_vm.h"
#include "f_init.h"
#include "f_utils.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    if (!vm || vm->process_count == 0 || argc < 1) return;

    FoxyProcess *p = vm->processes[vm->current_process_index];

    // 1. Recolectar todos los argumentos de la pila respetando LIFO
    FoxyValue *args = (FoxyValue *)malloc(sizeof(FoxyValue) * argc);
    if (!args) return;

    for (int i = argc - 1; i >= 0; i--) {
        args[i] = f_vm_pop(p);
    }

    // 2. El primer argumento (args[0]) es la cadena de formato
    const char *fmt = f_utils_get_string_from_constant(args[0]);
    if (!fmt) {
        f_utils_print_constant_dynamic(args[0]);
        free(args);
        return;
    }

    // 3. Procesar las marcas de formato y consumir args[1], args[2] en orden
    int arg_idx = 1;
    for (const char *ptr = fmt; *ptr != '\0'; ptr++) {
        if (*ptr == '%' && *(ptr + 1) != '\0') {
            ptr++; // Saltar el '%'
            
            if (arg_idx < argc) {
                f_utils_print_constant_dynamic(args[arg_idx]);
                arg_idx++;
            } else {
                f_utils_syswrite(1, "(null)", 6);
            }
        } else {
            f_utils_syswrite(1, ptr, 1);
        }
    }

    free(args);
}