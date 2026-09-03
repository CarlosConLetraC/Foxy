// f_include/f_openlib.h
#ifndef F_OPENLIB_H
    #define F_OPENLIB_H

    #include "f_settings.h"
    #include "f_vm.h"

    // Carga dinámica de librerías nativas compartidas (.so)
    bool f_vm_load_library(FoxyVM *vm, const char *lib_name);
#endif // F_OPENLIB_H
