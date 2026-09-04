// f_include/sys/out/f_init.h
#ifndef F_INIT_H
    #define F_INIT_H

    #define F_SYS_OUT_GET_CURRENT_PROCESS(vm) \
        ((vm && (vm)->process_count > 0) ? (vm)->processes[(vm)->current_process_index] : NULL)
    
    // Firmas alineadas con FoxyMethodFunc
    FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc);
    FOXY_EXPORT void f_sys_out_println(FoxyVM *vm, FoxyObject *self, int argc);
    FOXY_EXPORT void f_sys_out_printf(FoxyVM *vm, FoxyObject *self, int argc);

    // Punto de entrada global de la librería
    FOXY_EXPORT void foxy_init_module(FoxyVM *vm);
#endif // F_INIT_H