// f_include/sys/out/f_init.h
#ifndef F_INIT_H
    #define F_INIT_H

    #include "f_settings.h"
    #include "f_vm.h"
    #include "f_object.h"

    // Se obtiene el proceso de la VM o la cabecera de la tabla hash como fallback
    #define F_SYS_OUT_GET_CURRENT_PROCESS(vm) \
        ((vm) ? ((vm)->processes_hash) : NULL)
        
    FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *obj, int args);
    FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *obj, int args);
    FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *obj, int args);

    FOXY_EXPORT FoxyObject* f_out_module_init(void);
    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);

#endif // F_INIT_H