#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!proc || argc < 1) return;

    // El primer argumento (índice 0 de los argumentos) es la cadena de formato
    // En la pila, si se apilaron en orden, el formato está más abajo o más arriba según tu convención.
    // Asumiendo que el formato está en el índice 0 de los argumentos pasados:
    FoxyValue fmt_val = f_vm_peek(proc, argc - 1); // Ajusta el índice según el orden de tu pila
    const char *fmt = f_utils_get_string_from_constant(*(FoxyConstant*)&fmt_val);
    if (!fmt) return;

    int arg_index = 1; // Los valores para %T o %s empiezan a partir del siguiente argumento
    size_t i = 0;
    size_t fmt_len = strlen(fmt);

    while (i < fmt_len) {
        if (fmt[i] == '%' && i + 1 < fmt_len) {
            i++;
            char specifier = fmt[i];

            if (arg_index < argc) {
                // Obtener el argumento actual de la pila
                // (Calculando la posición relativa según argc y arg_index)
                FoxyValue arg_val = f_vm_peek(proc, argc - 1 - arg_index);

                switch (specifier) {
                    case 'T': {
                        f_utils_print_constant_dynamic(*(FoxyConstant*)&arg_val);
                        break;
                    }
                    case 's':
                    default:
                        f_utils_syswrite(1, "%", 1);
                        f_utils_syswrite(1, &specifier, 1);
                        break;
                }
                arg_index++;
            } else {
                f_utils_syswrite(1, "%", 1);
                f_utils_syswrite(1, &specifier, 1);
            }
        } else {
            f_utils_syswrite(1, &fmt[i], 1);
        }
        i++;
    }
}