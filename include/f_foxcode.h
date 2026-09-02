#ifndef F_FOXCODE_H
    #define F_FOXCODE_H

    #define FOXY_FOXCODE_LIST(F) \
        /* Control y Sistema */ \
        F(FOXCODE_NOP,          "NOP") \
        F(FOXCODE_HALT,         "HALT") \
        \
        /* Constantes y Literales */ \
        F(FOXCODE_LOAD_CONST,   "LOAD_CONST") \
        F(FOXCODE_LOAD_NULL,    "LOAD_NULL") \
        F(FOXCODE_LOAD_TRUE,    "LOAD_TRUE") \
        F(FOXCODE_LOAD_FALSE,   "LOAD_FALSE") \
        \
        /* Variables y Scopes (Locals/Globals/Libs) */ \
        F(FOXCODE_LOAD_LOCAL,   "LOAD_LOCAL") \
        F(FOXCODE_STORE_LOCAL,  "STORE_LOCAL") \
        F(FOXCODE_LOAD_GLOBAL,  "LOAD_GLOBAL") \
        F(FOXCODE_STORE_GLOBAL, "STORE_GLOBAL") \
        F(FOXCODE_LOAD_LIB,     "LOAD_LIB") \
        /* Propiedades y Objetos / Diccionarios */ \
        F(FOXCODE_GET_MEMBER,   "GET_MEMBER") \
        F(FOXCODE_SET_MEMBER,   "SET_MEMBER") \
        \
        /* Aritmética y Lógica */ \
        F(FOXCODE_ADD,          "ADD") \
        F(FOXCODE_SUB,          "SUB") \
        F(FOXCODE_MUL,          "MUL") \
        F(FOXCODE_DIV,          "DIV") \
        F(FOXCODE_EQ,           "EQ") \
        F(FOXCODE_NEQ,          "NEQ") \
        F(FOXCODE_LT,           "LT") \
        F(FOXCODE_GT,           "GT") \
        F(FOXCODE_LE,           "LE") \
        F(FOXCODE_GE,           "GE") \
        \
        /* Control de Flujo */ \
        F(FOXCODE_JUMP,         "JUMP") \
        F(FOXCODE_JUMP_IF_FALSE,"JUMP_IF_FALSE") \
        F(FOXCODE_CALL,         "CALL") \
        F(FOXCODE_RET,          "RET") \
        \
        /* Concurrencia y Procesos (Task Manager & Protocols) */ \
        F(FOXCODE_POPEN,        "POPEN")      /* Para spawnear subprocesos */ \
        F(FOXCODE_ENV,          "ENV")        /* Para registrar el entorno de foxy */ \
        F(FOXCODE_ENV_CREATE,   "ENV_CREATE") /* Para SharedEnv / protocols */ \
        F(FOXCODE_ENV_BIND,     "ENV_BIND")   /* Asociar proceso a un protocol */

    #define F(fcode, name) fcode,
    typedef enum __attribute__((__packed__)) {
        FOXY_FOXCODE_LIST(F)
        FOXCODE_COUNT
    } FOXY_FOXCODE;
    #undef F
#endif