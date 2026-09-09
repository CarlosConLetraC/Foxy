#ifndef F_CONSTANT_H
    #define F_CONSTANT_H
    #include <stdlib.h>
    #include "f_value.h"

    typedef struct FoxyVM FoxyVM;
    typedef FoxyValue FoxyConstant;
    
    typedef struct {
        FoxyValue *values;
        size_t count;
        size_t capacity;
    } FoxyConstantPool;

    void f_constant_pool_init(FoxyConstantPool *pool);
    size_t f_constant_pool_add(FoxyConstantPool *pool, FoxyValue value);
    void f_constant_pool_free(FoxyConstantPool *pool, FoxyVM *vm);
#endif // F_CONSTANT_H