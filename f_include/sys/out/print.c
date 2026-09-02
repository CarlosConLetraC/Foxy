#include "f_value.h"
#include "f_vm.h"
#include "f_init.h"
#include "f_utils.h"
#include <stdlib.h>

FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    if (!vm || vm->process_count == 0 || argc <= 0) return;

    FoxyProcess *p = vm->processes[vm->current_process_index];

    // Asignar buffer temporal para alinear los argumentos en el orden correcto de izquierda a derecha
    FoxyValue *args = (FoxyValue *)malloc(sizeof(FoxyValue) * argc);
    if (!args) return;

    // Desapilar en orden inverso (respetando la cima de la pila)
    for (int i = argc - 1; i >= 0; i--) {
        args[i] = f_vm_pop(p);
    }

    // Imprimir en el sentido natural
    for (int i = 0; i < argc; i++) {
        f_utils_print_constant_dynamic(args[i]);

        if (i < argc - 1) {
            f_utils_syswrite(1, " ", 1);
        }
    }

    free(args);
}