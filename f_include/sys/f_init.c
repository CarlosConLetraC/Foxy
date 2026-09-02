// sys/f_init.c
#include <stdio.h>
#include "f_vm.h"

// Punto de entrada estandarizado para el módulo raíz 'sys.so'
int f_module_init(FoxyVM* vm) {
    if (!vm) return -1;

    // El módulo raíz puede registrar utilidades generales del sistema operativo
    // o inicializar en cascada los submódulos dependientes.
    printf("[Foxy Native Module] Módulo raíz 'sys' inicializado correctamente.\n");
    return 0;
}