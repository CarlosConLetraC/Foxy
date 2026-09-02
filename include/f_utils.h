#ifndef F_UTILS_H
    #define F_UTILS_H
    #include "f_vm.h"
    #include "f_constant.h"

    char* f_utils_read_file(const char* filepath);
    void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name);

    // Funciones nativas operando directamente sobre FoxyConstant
    const char* f_utils_get_string_from_constant(FoxyConstant constant);
    long f_utils_get_long_from_constant(FoxyConstant constant);
    double f_utils_get_double_from_constant(FoxyConstant constant);
    void f_utils_print_constant_dynamic(FoxyConstant constant);

    // Utilidades del sistema sin libc
    int f_utils_int_to_ascii(long num, char *buf, unsigned long buffer_size);
    long f_utils_syswrite(int fd, const char *buf, unsigned long count);
#endif // F_UTILS_H