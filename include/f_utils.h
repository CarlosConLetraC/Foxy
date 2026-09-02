#ifndef F_UTILS_H
    #define F_UTILS_H
    #include "f_vm.h"
    typedef struct FoxyVM FoxyVM;
    char* f_utils_read_file(const char* filepath);
    void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name);
#endif