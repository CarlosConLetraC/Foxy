#include "f_settings.h"
#include "f_value.h"
#include "f_vm.h"
#include "f_object.h"
#include "f_class.h"
#include "f_dict.h"
#include "f_function.h"
#include "f_methods.h"
#include "f_utils.h"
#include "f_init.h"
#include <stdlib.h>
#include <stdio.h>

FOXY_EXPORT void f_sys_out_print(FoxyVM *vm, FoxyObject *self, int argc) {
    (void)self;
    if (!vm) return;

    FoxyProcess *proc = F_SYS_OUT_GET_CURRENT_PROCESS(vm);
    if (!proc || argc < 1) return;

    for (int i = 0; i < argc; i++) {
        FoxyValue val = f_vm_peek(proc, (size_t)(argc - 1 - i));

        switch (val.type) {
            case FOXY_VAL_OBJECT: {
                if (val.as.obj) {
                    FoxyClass *klass = (FoxyClass*)val.as.obj->klass;
                    const char *class_name = (klass && klass->name) ? klass->name : "Object";
                    char buf[128];
                    int len = snprintf(buf, sizeof(buf), "<instance of %s at %p>", class_name, (void*)val.as.obj);
                    if (len > 0) f_utils_syswrite(1, buf, (size_t)len);
                } else {
                    f_utils_syswrite(1, "null", 4);
                }
                break;
            }
            case FOXY_VAL_CLASS: {
                FoxyClass *klass = (FoxyClass*)val.as.klass;
                if (klass && klass->name) {
                    char buf[128];
                    int len = snprintf(buf, sizeof(buf), "<class %s>", klass->name);
                    if (len > 0) f_utils_syswrite(1, buf, (size_t)len);
                } else {
                    f_utils_syswrite(1, "<class>", 7);
                }
                break;
            }
            case FOXY_VAL_DICT: {
                if (val.as.dict) {
                    char buf[64];
                    int len = snprintf(buf, sizeof(buf), "<dict at %p>", (void*)val.as.dict);
                    if (len > 0) f_utils_syswrite(1, buf, (size_t)len);
                } else {
                    f_utils_syswrite(1, "null", 4);
                }
                break;
            }
            case FOXY_VAL_FUNCTION: {
                if (val.as.func && val.as.func->name) {
                    char buf[128];
                    int len = snprintf(buf, sizeof(buf), "<function %s>", val.as.func->name);
                    if (len > 0) f_utils_syswrite(1, buf, (size_t)len);
                } else {
                    f_utils_syswrite(1, "<function>", 10);
                }
                break;
            }
            default:
                f_utils_print_constant_dynamic(*(FoxyConstant*)&val, -1);
                break;
        }
    }
}