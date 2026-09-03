// f_include/sys/f_init.h
#ifndef F_SYS_INIT_H
    #define F_SYS_INIT_H

    #include "f_settings.h" // Asegura el uso de FOXY_EXPORT
    #include "f_vm.h"
    #include "f_value.h"

    // Punto de entrada global de la librería/módulo dinámico 'sys'
    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);
#endif // F_SYS_INIT_H