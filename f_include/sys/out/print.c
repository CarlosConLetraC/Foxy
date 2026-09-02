#include "f_init.h"
#include <stdio.h>

static const char* get_string_from_value(FoxyValue val) {
    if (val.type == FOXY_VAL_OBJECT && val.as.obj) {
        return (const char *)val.as.obj;
    }
    return NULL;
}

FOXY_EXPORT FoxyValue f_sys_out_print(int argc, FoxyValue *args) {
    if (!args) return FOXY_NULL_VALUE;

    for (int i = 0; i < argc; i++) {
        FoxyValue val = args[i];
        switch (val.type) {
            case FOXY_VAL_OBJECT: {
                const char *str = get_string_from_value(val);
                if (str) {
                    printf("%s", str);
                } else {
                    printf("[object]");
                }
                break;
            }
            case FOXY_VAL_INT:
                printf("%ld", (long)val.as.ival);
                break;
            case FOXY_VAL_BOOL:
                printf("%s", val.as.bval ? "true" : "false");
                break;
            case FOXY_VAL_NULL:
            default:
                printf("nil");
                break;
        }
    }

    fflush(stdout);
    return FOXY_NULL_VALUE;
}