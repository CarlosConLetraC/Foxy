#ifndef F_STATUS_H
    #define F_STATUS_H

    #define FOXY_STATUS_LIST(F) \
        F(FOXY_STATUS_SUCCESS,   "SUCCESS")       /* Ejecución exitosa */ \
        F(FOXY_STATUS_RUNTIME,   "RUNTIME")       /* Fallo general en tiempo de ejecución */ \
        F(FOXY_STATUS_MEMORY,    "MEMORY")        /* Error de asignación de memoria */ \
        F(FOXY_STATUS_TYPE,      "TYPE")          /* Incompatibilidad de tipos en la VM */ \
        F(FOXY_STATUS_BOUNDS,    "BOUNDS")        /* Índice fuera de límites */ \
        F(FOXY_STATUS_IO,        "IO")            /* Error de entrada/salida o módulos */

    #define F(code, name) code,
    typedef enum __attribute__((__packed__)) {
        FOXY_STATUS_LIST(F)
        FOXY_STATUS_COUNT
    } FoxyStatus;
    #undef F

    const char* f_status_to_string(FoxyStatus status);
#endif // F_STATUS_H