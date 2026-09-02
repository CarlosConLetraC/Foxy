#ifndef FOXY_TOKEN_H
    #define FOXY_TOKEN_H
    #include <stdint.h>

    // 1. Listas maestras (X-Macros)
    #define FOXY_TOKEN_LIST(F) \
        F(FOXY_TOKEN_FUNCTION) \
        F(FOXY_TOKEN_CLASS) \
        F(FOXY_TOKEN_FROM) \
        F(FOXY_TOKEN_STRUCT) \
        F(FOXY_TOKEN_ENUM) \
        F(FOXY_TOKEN_EXPORT) \
        F(FOXY_TOKEN_USE) \
        F(FOXY_TOKEN_OVERRULE) \
        F(FOXY_TOKEN_SUPER) \
        F(FOXY_TOKEN_SELF) \
        F(FOXY_TOKEN_FOR) \
        F(FOXY_TOKEN_IF) \
        F(FOXY_TOKEN_SWITCH) \
        F(FOXY_TOKEN_CASE) \
        F(FOXY_TOKEN_DEFAULT) \
        F(FOXY_TOKEN_BREAK) \
        F(FOXY_TOKEN_RETURN) \
        F(FOXY_TOKEN_INCLUDE) \
        F(FOXY_TOKEN_PRIVATE) \
        F(FOXY_TOKEN_PROTECTED) \
        F(FOXY_TOKEN_PUBLIC)

    #define FOXY_TOKEN_TYPE_LIST(F) \
        F(FOXY_TOKEN_TYPE_NULL) \
        F(FOXY_TOKEN_TYPE_CHAR) \
        F(FOXY_TOKEN_TYPE_INT) \
        F(FOXY_TOKEN_TYPE_NUMBER) \
        F(FOXY_TOKEN_TYPE_FLOAT) \
        F(FOXY_TOKEN_TYPE_DOUBLE) \
        F(FOXY_TOKEN_TYPE_LONG) \
        F(FOXY_TOKEN_TYPE_LONG_LONG) \
        F(FOXY_TOKEN_TYPE_UNSIGNED_LONG_LONG) \
        F(FOXY_TOKEN_TYPE_BOOL) \
        F(FOXY_TOKEN_TYPE_OBJECT) \
        F(FOXY_TOKEN_TYPE_CLASS) \
        F(FOXY_TOKEN_TYPE_DICT) \
        F(FOXY_TOKEN_TYPE_STRUCT) \
        F(FOXY_TOKEN_TYPE_FUNCTION)

    #define FOXY_TOKEN_IDENTIFIER_LIST(F) \
        F(FOXY_TOKEN_IDENTIFIER_NAME) \
        F(FOXY_TOKEN_IDENTIFIER_CHAR)               /* '' unicamente */ \
        F(FOXY_TOKEN_IDENTIFIER_NUMBER) \
        F(FOXY_TOKEN_IDENTIFIER_INT) \
        F(FOXY_TOKEN_IDENTIFIER_FLOAT) \
        F(FOXY_TOKEN_IDENTIFIER_DOUBLE) \
        F(FOXY_TOKEN_IDENTIFIER_LONG) \
        F(FOXY_TOKEN_IDENTIFIER_LONG_LONG) \
        F(FOXY_TOKEN_IDENTIFIER_UNSIGNED_LONG_LONG) \
        F(FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY)         /* "" unicamente */

    #define FOXY_TOKEN_OPERATOR_LIST(F) \
        F(FOXY_TOKEN_OPERATOR_ASSIGN) \
        F(FOXY_TOKEN_OPERATOR_ADD) \
        F(FOXY_TOKEN_OPERATOR_SUB) \
        F(FOXY_TOKEN_OPERATOR_MUL) \
        F(FOXY_TOKEN_OPERATOR_DIV) \
        F(FOXY_TOKEN_OPERATOR_POW) \
        /* Operadores Lógicos y de Bits */ \
        F(FOXY_TOKEN_OPERATOR_AND) \
        F(FOXY_TOKEN_OPERATOR_OR)  \
        F(FOXY_TOKEN_OPERATOR_BAND) \
        F(FOXY_TOKEN_OPERATOR_BOR)  \
        F(FOXY_TOKEN_OPERATOR_BXOR) \
        F(FOXY_TOKEN_OPERATOR_BNOT) \
        F(FOXY_TOKEN_OPERATOR_NOT)  \
        F(FOXY_TOKEN_OPERATOR_SHL)  \
        F(FOXY_TOKEN_OPERATOR_SHR)  \
        /* Operadores de Comparación y Relacionales */ \
        F(FOXY_TOKEN_OPERATOR_EQ)  \
        F(FOXY_TOKEN_OPERATOR_NEQ) \
        F(FOXY_TOKEN_OPERATOR_LT)  \
        F(FOXY_TOKEN_OPERATOR_GT)  \
        F(FOXY_TOKEN_OPERATOR_LE)  \
        F(FOXY_TOKEN_OPERATOR_GE)  \
        /* Operadores Especiales y Condicionales */ \
        F(FOXY_TOKEN_OPERATOR_QUESTION) \
        /* Resto de operadores */ \
        F(FOXY_TOKEN_OPERATOR_ARROW) \
        F(FOXY_TOKEN_OPERATOR_DOT) \
        F(FOXY_TOKEN_OPERATOR_HASH) \
        F(FOXY_TOKEN_OPERATOR_COLON) \
        F(FOXY_TOKEN_OPERATOR_SEMICOLON) \
        F(FOXY_TOKEN_OPERATOR_COMMA) \
        F(FOXY_TOKEN_OPERATOR_LPAREN) \
        F(FOXY_TOKEN_OPERATOR_RPAREN) \
        F(FOXY_TOKEN_OPERATOR_LBRACE) \
        F(FOXY_TOKEN_OPERATOR_RBRACE) \
        F(FOXY_TOKEN_OPERATOR_LBRACKET) \
        F(FOXY_TOKEN_OPERATOR_RBRACKET) \
        F(FOXY_TOKEN_OPERATOR_LAMBDA) \
        F(FOXY_TOKEN_OPERATOR_ELLIPSIS)

    #define FOXY_TOKEN_FLAGGED_METHOD_LIST(F) \
        F(FOXY_TOKEN_FLAGGED_METHOD_NEW)          /* __new (Constructor) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_TOSTRING)     /* __tostring */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_ADD)          /* __add (+) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_SUB)          /* __sub (-) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_MUL)          /* __mul (*) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_DIV)          /* __div (/) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_POW)          /* __pow (**) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_BAND)         /* __band (&) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_BOR)          /* __bor (|) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_BNOT)         /* __bnot (~) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_BXOR)         /* __bxor (^) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_LSHIFT)       /* __lshift (<<) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_RSHIFT)       /* __rshift (>>) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_CLOSED)       /* __closed (Garbage collector / cierre de campo) */ \
        F(FOXY_TOKEN_FLAGGED_METHOD_LEN)          /* __len (#) */

    #define FOXY_TOKEN_ERROR_LIST(F) \
        F(FOXY_TOKEN_ERROR_SYNTAX) \
        F(FOXY_TOKEN_ERROR_RUNTIME) \
        F(FOXY_TOKEN_ERROR_ARITHMETIC) \
        F(FOXY_TOKEN_ERROR_CAST)                 /* Fallo de cast (ej: (int)<class object>, (string)float) */ \
        F(FOXY_TOKEN_ERROR_OUT_OF_RANGE)         /* Arreglos de tamaño fijo o índices fuera de límites */ \
        F(FOXY_TOKEN_ERROR_VARIABLE_UNDEFINED)   /* Variable fuera de scope o sin definir */ \
        F(FOXY_TOKEN_ERROR_NULL)                 /* Datos o punteros no inicializados */ \
        F(FOXY_TOKEN_ERROR_HAS_NO_ATTRIBUTE)     /* Atributo/método inexistente en módulo o clase */

    // 2. Generación de Enums compactos (__attribute__((__packed__)) para ahorrar memoria)
    #define F(name) name,

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_LIST(F)
    } FOXY_TOKEN;

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_TYPE_LIST(F)
    } FOXY_TOKEN_TYPE;

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_IDENTIFIER_LIST(F)
    } FOXY_TOKEN_IDENTIFIER;

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_OPERATOR_LIST(F)
    } FOXY_TOKEN_OPERATOR;

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_FLAGGED_METHOD_LIST(F)
    } FOXY_TOKEN_FLAGGED_METHOD;

    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_ERROR_LIST(F)
    } FOXY_TOKEN_ERROR;

    #undef F
#endif // FOXY_TOKEN_H
