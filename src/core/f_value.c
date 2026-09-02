#include "f_value.h"

static const char * const FOXY_VALUE_TYPE_STRINGS[] = {
    #define F(type_enum, type_str) type_str,
    FOXY_VALUE_TYPE_LIST(F)
    #undef F
};

const char* f_value_type_to_string(FoxyValueType type) {
    if ((unsigned int)type >= FOXY_VAL_COUNT) return "unknown";
    return FOXY_VALUE_TYPE_STRINGS[type];
}