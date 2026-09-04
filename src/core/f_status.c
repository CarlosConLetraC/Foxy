#include "f_status.h"

const char* f_status_to_string(FoxyStatus status) {
    switch (status) {
        #define F(code, name) case code: return name;
        FOXY_STATUS_LIST(F)
        #undef F
        default: return "UNKNOWN_STATUS";
    }
}