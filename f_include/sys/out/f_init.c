#ifndef F_INIT_H
    #define F_INIT_H

    #include "f_vm.h"
    #include "f_value.h"
    #include "f_settings.h"

    #if defined(__GNUC__) || defined(__clang__)
      #define FOXY_EXPORT __attribute__((visibility("default")))
    #else
      #define FOXY_EXPORT
    #endif

    FOXY_EXPORT FoxyValue f_sys_out_print(int argc, FoxyValue *args);
    FOXY_EXPORT FoxyValue f_sys_out_println(int argc, FoxyValue *args);
    FOXY_EXPORT FoxyValue f_sys_out_printf(int argc, FoxyValue *args);

    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);

#endif // F_INIT_H