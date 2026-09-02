#ifndef F_INIT_H
    #define F_INIT_H

    #include "f_settings.h"
    #include "f_vm.h"
    #include "f_value.h"

    // Firmas alineadas con FoxyMethodFunc
    FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc);
    FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc);
    FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc);

    // Punto de entrada global de la librería
    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);
#endif // F_INIT_H