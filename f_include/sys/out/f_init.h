#ifndef F_INIT_H
    #define F_INIT_H

    #include "f_settings.h" // Asegura que FOXY_EXPORT esté disponible
    #include "f_vm.h"
    #include "f_value.h"

    // Firmas actualizadas a la nueva arquitectura
    FOXY_EXPORT FoxyValue f_sys_out_print(int argc, FoxyValue *args);
    FOXY_EXPORT FoxyValue f_sys_out_println(int argc, FoxyValue *args);
    FOXY_EXPORT FoxyValue f_sys_out_printf(int argc, FoxyValue *args);

    // Punto de entrada global de la librería
    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);
#endif // F_INIT_H