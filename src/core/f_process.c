#include "f_process.h"

// Generación automática del arreglo de strings usando la misma X-Macro
static const char* FOXY_PROCESS_STATE_STRINGS[] = {
    #define F(state, str) str,
    FOXY_PROCESS_STATE_LIST(F)
    #undef F
};

const char* f_process_state_to_string(FoxyProcessState state) {
    if (state < FOXY_PROCESS_STATE_COUNT)
        return FOXY_PROCESS_STATE_STRINGS[state];
    return "UNKNOWN_STATE";
}