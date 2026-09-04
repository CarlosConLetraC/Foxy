#ifndef FOXY_TOKEN_H
    #define FOXY_TOKEN_H

    #include <stddef.h>
    #include <stdint.h>
    #include "f_foxmode.h"

    // Categorías principales de Tokens
    #define FOXY_TOKEN_CAT_KEYWORD     1
    #define FOXY_TOKEN_CAT_TYPE        2
    #define FOXY_TOKEN_CAT_IDENTIFIER  3
    #define FOXY_TOKEN_CAT_OPERATOR    4
    #define FOXY_TOKEN_CAT_METHOD      5
    #define FOXY_TOKEN_CAT_LITERAL     6
    #define FOXY_TOKEN_CAT_ERROR       7

    // 1. Listas maestras (X-Macros)
    #define FOXY_TOKEN_LIST(F) \
        F(FOXY_TOKEN_LIST_FUNCTION) \
        F(FOXY_TOKEN_LIST_CLASS) \
        F(FOXY_TOKEN_LIST_FROM) \
        F(FOXY_TOKEN_LIST_STRUCT) \
        F(FOXY_TOKEN_LIST_ENUM) \
        F(FOXY_TOKEN_LIST_EXPORT) \
        F(FOXY_TOKEN_LIST_USE) \
        F(FOXY_TOKEN_LIST_OVERRULE) \
        F(FOXY_TOKEN_LIST_SUPER) \
        F(FOXY_TOKEN_LIST_SELF) \
        F(FOXY_TOKEN_LIST_FOR) \
        F(FOXY_TOKEN_LIST_IF) \
        F(FOXY_TOKEN_LIST_SWITCH) \
        F(FOXY_TOKEN_LIST_CASE) \
        F(FOXY_TOKEN_LIST_DEFAULT) \
        F(FOXY_TOKEN_LIST_BREAK) \
        F(FOXY_TOKEN_LIST_RETURN) \
        F(FOXY_TOKEN_LIST_INCLUDE) \
        F(FOXY_TOKEN_LIST_PRIVATE) \
        F(FOXY_TOKEN_LIST_PROTECTED) \
        F(FOXY_TOKEN_LIST_PUBLIC)

    #define FOXY_TOKEN_TYPE_LIST(F) \
        F(FOXY_TOKEN_TYPE_NULL) \
        F(FOXY_TOKEN_TYPE_VOID) \
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
        F(FOXY_TOKEN_OPERATOR_MOD) \
        F(FOXY_TOKEN_OPERATOR_POW) \
        F(FOXY_TOKEN_OPERATOR_INC) \
        F(FOXY_TOKEN_OPERATOR_DEC) /*NUEVO: decremento (i--, --i)*/\
        F(FOXY_TOKEN_OPERATOR_NEG) \
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
        F(FOXY_TOKEN_ERROR_SYNTAX,              "SYNTAX")              /* Fallo de sintaxis */ \
        F(FOXY_TOKEN_ERROR_RUNTIME,             "RUNTIME")             /* Fallo de ejecución */ \
        F(FOXY_TOKEN_ERROR_ARITHMETIC,          "ARITHMETIC")          /* Error aritmético */ \
        F(FOXY_TOKEN_ERROR_CAST,                "CAST")                /* Fallo de cast */ \
        F(FOXY_TOKEN_ERROR_OUT_OF_RANGE,        "OUT_OF_RANGE")        /* Índices fuera de límites */ \
        F(FOXY_TOKEN_ERROR_VARIABLE_UNDEFINED,  "VARIABLE_UNDEFINED")  /* Variable sin definir */ \
        F(FOXY_TOKEN_ERROR_NULL,                "NULL")                /* Punteros no inicializados */ \
        F(FOXY_TOKEN_ERROR_HAS_NO_ATTRIBUTE,    "HAS_NO_ATTRIBUTE")    /* Objeto sin atributo */

    // 2. Generación de Enums compactos (__attribute__((__packed__)))

    #define F(code, name) code,
    typedef enum __attribute__((__packed__)) {
        FOXY_TOKEN_ERROR_LIST(F)
        FOXY_TOKEN_ERROR_COUNT
    } FoxyErrorType;
    #undef F

    typedef enum __attribute__((__packed__)) {
        #define F(name) name,
        FOXY_TOKEN_LIST(F)
        #undef F
    } FOXY_TOKEN;

    typedef enum __attribute__((__packed__)) {
        #define F(name) name,
        FOXY_TOKEN_TYPE_LIST(F)
        #undef F
    } FOXY_TOKEN_TYPE;

    typedef enum __attribute__((__packed__)) {
        #define F(name) name,
        FOXY_TOKEN_IDENTIFIER_LIST(F)
        #undef F
    } FOXY_TOKEN_IDENTIFIER;

    typedef enum __attribute__((__packed__)) {
        #define F(name) name,
        FOXY_TOKEN_OPERATOR_LIST(F)
        #undef F
    } FOXY_TOKEN_OPERATOR;

    typedef enum __attribute__((__packed__)) {
        #define F(name) name,
        FOXY_TOKEN_FLAGGED_METHOD_LIST(F)
        #undef F
    } FOXY_TOKEN_FLAGGED_METHOD;

    typedef struct {
        const char* start;              // Puntero al lexema en el fuente
        uint32_t length;                // Longitud del lexema
        uint16_t line;                  // Línea de origen
        uint16_t column;                // Columna de origen
        unsigned int type_category : 4; // Categoría principal (0-15)
        unsigned int subtype : 12;      // Subtipo del enum correspondiente (0-4095)
        
        // Unión para almacenar valores evaluados directamente en el lexer
        union {
            int64_t int_val;
            double float_val;
            void* custom_data;
        } as;
    } FoxyToken;

    typedef struct {
        const char* text;            // Nombre de la palabra reservada
        unsigned int category : 4;   // Categoría (1: Keyword, 2: Primitive Type)
        unsigned int subtype : 12;    // Valor enum asignado
    } FoxyKeywordMap;

    // TODO: Renombrar KEYWORD_TABLE a FOXY_KEYWORD_TABLE para futuras anotaciones. . .
    static const FoxyKeywordMap KEYWORD_TABLE[] = {
        // Categoría 1: Keywords de Estructura y Control (FOXY_TOKEN)
        { "function",  FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_FUNCTION },
        { "class",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_CLASS },
        { "from",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_FROM },
        { "struct",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_STRUCT },
        { "enum",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_ENUM },
        { "export",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_EXPORT },
        { "use",       FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_USE },
        { "overrule",  FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_OVERRULE },
        { "super",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_SUPER },
        { "self",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_SELF },
        { "for",       FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_FOR },
        { "if",        FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_IF },
        { "switch",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_SWITCH },
        { "case",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_CASE },
        { "default",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_DEFAULT },
        { "break",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_BREAK },
        { "return",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_RETURN },
        { "include",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_INCLUDE },
        { "private",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_PRIVATE },
        { "protected", FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_PROTECTED },
        { "public",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_LIST_PUBLIC },

        // Categoría 2: Tipos de datos primitivos (FOXY_TOKEN_TYPE)
        { "char",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_CHAR },
        { "int",       FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_INT },
        { "number",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_NUMBER },
        { "float",     FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_FLOAT },
        { "double",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_DOUBLE },
        { "long",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_LONG },
        { "bool",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_BOOL },
        { "object",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_OBJECT },
        { "dict",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_DICT },

        // Literales Booleanos
        { "true",      FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_TYPE_BOOL },
        { "false",     FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_TYPE_BOOL }
    };

    static const size_t KEYWORD_TABLE_SIZE = sizeof(KEYWORD_TABLE) / sizeof(FoxyKeywordMap);
#endif // FOXY_TOKEN_H