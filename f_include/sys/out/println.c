#include <stdio.h>
#include "f_vm.h"

int f_sys_println(FoxyVM* vm) {
    (void)vm;
    // Aquí más adelante extraeremos el argumento real de la pila de la VM.
    // Por ahora, ejecutamos la acción nativa de impresión con salto de línea.
    printf("[Foxy Native] println ejecutado\n");
    return 0;
}