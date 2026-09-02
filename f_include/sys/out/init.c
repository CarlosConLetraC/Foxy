#include <stdio.h>
#include "f_vm.h"

// Declaración de las funciones nativas implementadas en este directorio
extern int f_sys_print(FoxyVM* vm);
extern int f_sys_printf(FoxyVM* vm);
extern int f_sys_println(FoxyVM* vm);

// Punto de entrada estandarizado que buscará la VM con dlsym()
int f_module_init(FoxyVM* vm) {
    if (!vm) return -1;

    // Aquí registrarás las funciones en el entorno global o tabla de símbolos de la VM.
    // Ejemplo conceptual:
    // f_vm_register_native(vm, "sys.out.print", f_sys_print);
    // f_vm_register_native(vm, "sys.out.println", f_sys_println);

    printf("[Foxy Native Module] Módulo 'sys/out' inicializado correctamente.\n");
    return 0;
}