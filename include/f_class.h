#ifndef F_CLASS_H
    #define F_CLASS_H

    #include <stddef.h>
    #include "f_methods.h"

    typedef struct FoxyClass {
        const char *name;
        struct FoxyClass *super_class;
        FoxyMethod *methods;
        size_t method_count;
        size_t method_capacity;
    } FoxyClass;

    FoxyClass* f_class_new(const char *name, FoxyClass *super_class);
    void f_class_add_method_native(FoxyClass *klass, const char *name, FoxyMethodFunc func);
    FoxyMethod* f_class_find_method(FoxyClass *klass, const char *name);
    void f_class_free(FoxyClass *klass);
#endif // F_CLASS_H