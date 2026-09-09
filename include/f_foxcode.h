#ifndef F_FOXCODE_H
    #define F_FOXCODE_H
    #include <stdint.h>

    #define FOXY_FOXCODE_LIST(F) \
        /* Control y Sistema */ \
        F(FOXCODE_NOP,           "NOP",           "nop") \
        F(FOXCODE_HALT,          "HALT",          "halt") \
        F(FOXCODE_INCLUDE,       "INCLUDE",       "include") \
        F(FOXCODE_COLLECT,       "COLLECT",       "collect") \
        \
        /* Constantes y Literales */ \
        F(FOXCODE_LOAD_CONST,    "LOAD_CONST",    "load_const") \
        F(FOXCODE_LOAD_NULL,     "LOAD_NULL",     "null") \
        F(FOXCODE_LOAD_TRUE,     "LOAD_TRUE",     "true") \
        F(FOXCODE_LOAD_FALSE,    "LOAD_FALSE",    "false") \
        \
        /* Variables y Scopes */ \
        F(FOXCODE_LOAD_LOCAL,    "LOAD_LOCAL",    "load_local") \
        F(FOXCODE_STORE_LOCAL,   "STORE_LOCAL",   "store_local") \
        F(FOXCODE_LOAD_GLOBAL,   "LOAD_GLOBAL",   "load_global") \
        F(FOXCODE_STORE_GLOBAL,  "STORE_GLOBAL",  "store_global") \
        F(FOXCODE_LOAD_LIB,      "LOAD_LIB",      "load_lib") \
        \
        /* Propiedades y Miembros */ \
        F(FOXCODE_GET_MEMBER,     "GET_MEMBER",     ".") \
        F(FOXCODE_SET_MEMBER,     "SET_MEMBER",     ".=") \
        F(FOXCODE_GET_MEMBER_PTR, "GET_MEMBER_PTR", "->") \
        F(FOXCODE_SET_MEMBER_PTR, "SET_MEMBER_PTR", "->=") \
        \
        /* Indexación de Arreglos y Diccionarios */ \
        F(FOXCODE_GET_INDEX,     "GET_INDEX",     "[]") \
        F(FOXCODE_SET_INDEX,     "SET_INDEX",     "[]=") \
        \
        /* Control de Ciclos e Iteradores */ \
        F(FOXCODE_WHILE_ITER,    "WHILE_ITER",    "while_iter") \
        F(FOXCODE_FOR_ITER,      "FOR_ITER",      "for_iter") \
        F(FOXCODE_FOR_NEXT,      "FOR_NEXT",      "for_next") \
        F(FOXCODE_FOREACH_CALL,  "FOREACH_CALL",  "foreach_call") \
        \
        /* Aritmética y Comparaciones */ \
        F(FOXCODE_ADD,           "ADD",           "+") \
        F(FOXCODE_SUB,           "SUB",           "-") \
        F(FOXCODE_MUL,           "MUL",           "*") \
        F(FOXCODE_DIV,           "DIV",           "/") \
        F(FOXCODE_MOD,           "MOD",           "%") \
        F(FOXCODE_NEG,           "NEG",           "-") \
        F(FOXCODE_NOT,           "NOT",           "!") \
        F(FOXCODE_EQ,            "EQ",            "==") \
        F(FOXCODE_NEQ,           "NEQ",           "!=") \
        F(FOXCODE_LT,            "LT",            "<") \
        F(FOXCODE_GT,            "GT",            ">") \
        F(FOXCODE_LE,            "LE",            "<=") \
        F(FOXCODE_GE,            "GE",            ">=") \
        \
        /* Operadores Bitwise */ \
        F(FOXCODE_BIT_AND,       "BIT_AND",       "&") \
        F(FOXCODE_BIT_OR,        "BIT_OR",        "|") \
        F(FOXCODE_BIT_XOR,       "BIT_XOR",       "^") \
        F(FOXCODE_BIT_NOT,       "BIT_NOT",       "~") \
        F(FOXCODE_BIT_SHL,       "SHL",           "<<") \
        F(FOXCODE_BIT_SHR,       "SHR",           ">>") \
        \
        /* Control de Flujo */ \
        F(FOXCODE_JUMP,          "JUMP",          "jump") \
        F(FOXCODE_JUMP_IF_FALSE, "JUMP_IF_FALSE", "jump_if_false") \
        F(FOXCODE_POP,           "FOXCODE_POP",   "pop") \
        F(FOXCODE_CALL,          "CALL",          "call") \
        F(FOXCODE_RET,           "RET",           "ret") \
        \
        /* Construcción de Estructuras y POO */ \
        F(FOXCODE_NEW_ARRAY,     "NEW_ARRAY",     "new_array") \
        F(FOXCODE_NEW_DICT,      "NEW_DICT",      "new_dict") \
        F(FOXCODE_CLASS_NEW,     "CLASS_NEW",     "class_new") \
        F(FOXCODE_NEW_OBJECT,    "NEW_OBJECT",    "new_object") \
        F(FOXCODE_METHOD_BIND,   "METHOD_BIND",   "method_bind") \
        \
        /* Concurrencia y Procesos */ \
        F(FOXCODE_POPEN,         "POPEN",         "popen") \
        F(FOXCODE_ENV,           "ENV",           "env") \
        F(FOXCODE_ENV_CREATE,    "ENV_CREATE",    "env_create") \
        F(FOXCODE_ENV_BIND,      "ENV_BIND",      "env_bind") \
        \
        /* Control de excepciones */

        /*
        TODO: implementar estos foxcode:
        lbl_FOXCODE_PUSH_LIST:
        lbl_FOXCODE_PUSH_DICT:
        lbl_FOXCODE_SUPER_CALL:
        lbl_FOXCODE_THROW:
        lbl_FOXCODE_TRY_BEGIN:
        lbl_FOXCODE_TRY_END: {
            VM_LOG("Opcode extendido o reservado alcanzado: %s", CURRENT_OP_NAME());
            DISPATCH();
        }
        */

    #define F(fcode, name, symbol) fcode,
    typedef enum __attribute__((__packed__)) {
        FOXY_FOXCODE_LIST(F)
        FOXCODE_COUNT
    } FOXY_FOXCODE;
    #undef F

    typedef FOXY_FOXCODE FoxOpcode;
    // typedef FOXY_FOXCODE FoxyOpcode;

#endif /* F_FOXCODE_H */