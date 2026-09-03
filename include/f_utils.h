#ifndef F_UTILS_H
    #define F_UTILS_H

    #include <stdio.h>
    #include <stdarg.h>
    #include "f_vm.h"
    #include "f_constant.h"
    #include "f_token.h"
    
    char* f_utils_read_file(const char* filepath);
    void f_utils_load_native_lib(FoxyVM* vm, const char* lib_name);

    const char* f_utils_get_string_from_constant(FoxyConstant constant);
    long f_utils_get_long_from_constant(FoxyConstant constant);
    double f_utils_get_double_from_constant(FoxyConstant constant);
    void f_utils_print_constant_dynamic(FoxyConstant constant);

    int f_utils_int_to_ascii(long num, char *buf, unsigned long buffer_size);
    long f_utils_syswrite(int fd, const char *buf, unsigned long count);

    const char* f_utils_error_to_string(FoxyErrorType error);
    size_t f_utils_unescape_string(const char *src, size_t src_len, char *dest, size_t dest_size);
    void f_utils_write_runtime_error(struct FoxyVM *vm, FoxyErrorType err_type, const char *format, ...);
#endif // F_UTILS_H