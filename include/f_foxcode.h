#ifndef F_FOXCODE_H
    #define F_FOXCODE_H
    #include <stdint.h>

    #define FOXY_FOXCODE_LIST(F) \
        /* Control y Sistema */ \
        F(FOXCODE_NOP,           "NOP",           "nop") \
        F(FOXCODE_HALT,          "HALT",          "halt") \
        F(FOXCODE_INCLUDE,       "INCLUDE",       "include") \
        \
        /* Constantes y Literales */ \
        F(FOXCODE_LOAD_CONST,    "LOAD_CONST",    "load_const") \
        F(FOXCODE_LOAD_NULL,     "LOAD_NULL",     "null") \
        F(FOXCODE_LOAD_TRUE,     "LOAD_TRUE",     "true") \
        F(FOXCODE_LOAD_FALSE,    "LOAD_FALSE",    "false") \
        \
        /* Variables y Scopes (Locals/Globals/Libs) */ \
        F(FOXCODE_LOAD_LOCAL,    "LOAD_LOCAL",    "load_local") \
        F(FOXCODE_STORE_LOCAL,   "STORE_LOCAL",   "store_local") \
        F(FOXCODE_LOAD_GLOBAL,   "LOAD_GLOBAL",   "load_global") \
        F(FOXCODE_STORE_GLOBAL,  "STORE_GLOBAL",  "store_global") \
        F(FOXCODE_LOAD_LIB,      "LOAD_LIB",      "load_lib") \
        \
        /* Propiedades, Objetos, Diccionarios y Estructuras */ \
        F(FOXCODE_GET_MEMBER,     "GET_MEMBER",     ".") \
        F(FOXCODE_SET_MEMBER,     "SET_MEMBER",     ".=") \
        F(FOXCODE_GET_MEMBER_PTR, "GET_MEMBER_PTR", "->") \
        F(FOXCODE_SET_MEMBER_PTR, "SET_MEMBER_PTR", "->=") \
        \
        /* Control de Ciclos e Iteradores */ \
        F(FOXCODE_FOR_ITER,      "FOR_ITER",      "for_iter") \
        F(FOXCODE_FOR_NEXT,      "FOR_NEXT",      "for_next") \
        F(FOXCODE_FOREACH_CALL,  "FOREACH_CALL",  "foreach_call") \
        \
        /* Aritmética y Comparaciones */ \
        F(FOXCODE_ADD,           "ADD",           "+") \
        F(FOXCODE_SUB,           "SUB",           "-") \
        F(FOXCODE_MUL,           "MUL",           "*") \
        F(FOXCODE_DIV,           "DIV",           "/") \
        F(FOXCODE_EQ,            "EQ",            "==") \
        F(FOXCODE_NEQ,           "NEQ",           "!=") \
        F(FOXCODE_LT,            "LT",            "<") \
        F(FOXCODE_GT,            "GT",            ">") \
        F(FOXCODE_LE,            "LE",            "<=") \
        F(FOXCODE_GE,            "GE",            ">=") \
        \
        /* Operadores Bitwise / Lógicos de Bajo Nivel */ \
        F(FOXCODE_BIT_AND,       "BIT_AND",       "&") \
        F(FOXCODE_BIT_OR,        "BIT_OR",        "|") \
        F(FOXCODE_BIT_XOR,       "BIT_XOR",       "^") \
        F(FOXCODE_BIT_NOT,       "BIT_NOT",       "~") \
        F(FOXCODE_SHL,           "SHL",           "<<") \
        F(FOXCODE_SHR,           "SHR",           ">>") \
        \
        /* Control de Flujo */ \
        F(FOXCODE_JUMP,          "JUMP",          "jump") \
        F(FOXCODE_JUMP_IF_FALSE, "JUMP_IF_FALSE", "jump_if_false") \
        F(FOXCODE_POP,           "FOXCODE_POP",   "pop") \
        F(FOXCODE_CALL,          "CALL",          "call") \
        F(FOXCODE_RET,           "RET",           "ret") \
        \
        /* Concurrencia y Procesos (Task Manager & Protocols) */ \
        F(FOXCODE_POPEN,         "POPEN",         "popen")      /* Spawnear subprocesos */ \
        F(FOXCODE_ENV,           "ENV",           "env")        /* Registrar el entorno foxy */ \
        F(FOXCODE_ENV_CREATE,    "ENV_CREATE",    "env_create") /* SharedEnv / protocols */ \
        F(FOXCODE_ENV_BIND,      "ENV_BIND",      "env_bind")   /* Asociar proceso a protocol */

    #define F(fcode, name, symbol) fcode,
    typedef enum __attribute__((__packed__)) {
        FOXY_FOXCODE_LIST(F)
        FOXCODE_COUNT
    } FOXY_FOXCODE;
    #undef F

    /* Alias para compatibilidad con el codegen y la máquina virtual */
    typedef FOXY_FOXCODE FoxOpcode;
    typedef FOXY_FOXCODE FoxyOpcode;

#endif /* F_FOXCODE_H */