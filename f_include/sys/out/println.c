#include "f_value.h"
#include "f_vm.h"
#include "f_init.h"
#include "f_utils.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    if (!vm || vm->process_count == 0 || argc <= 0) {
        f_utils_syswrite(1, "\n", 1);
        return;
    }

    FoxyProcess *p = vm->processes[vm->current_process_index];

    // Asignar un arreglo temporal para ordenar los argumentos en el sentido correcto
    // (O usar un array estático pequeño si el límite de argumentos por línea es razonable, ej. 32)
    FoxyValue *args = (FoxyValue *)malloc(sizeof(FoxyValue) * argc);
    if (!args) return;

    // Desapilar en orden inverso (porque el último argumento empujado está arriba de la pila)
    for (int i = argc - 1; i >= 0; i--) {
        args[i] = f_vm_pop(p);
    }

    // Imprimir en el orden natural de izquierda a derecha
    for (int i = 0; i < argc; i++) {
        f_utils_print_constant_dynamic(args[i]);

        if (i < argc - 1) {
            f_utils_syswrite(1, " ", 1);
        }
    }

    f_utils_syswrite(1, "\n", 1);

    free(args);
}