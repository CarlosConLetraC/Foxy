#include "f_token.h"

/* ========================================================================= */
/* TABLA DE PALABRAS RESERVADAS (BÚSQUEDA / MAPEO LÉXICO)                    */
/* ========================================================================= */

const FoxyKeywordMap FOXY_KEYWORD_TABLE[] = {
    /* Tipos de Datos Primitivos */
    { "null",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_NULL },
    { "bool",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_BOOL },
    { "char",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_CHAR },
    { "uchar",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_UCHAR },
    { "short",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_SHORT },
    { "ushort",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_USHORT },
    { "int",          FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_INT },
    { "uint",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_UINT },
    { "long",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LONG },
    { "ulong",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_ULONG },
    { "llong",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LLONG },
    { "ullong",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_ULLONG },
    { "float",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_FLOAT },
    { "double",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_DOUBLE },
    { "ldouble",      FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LDOUBLE },
    { "number",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_NUMBER },
    { "dict",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_DICT },
    { "object",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_OBJECT },

    /* Control de Flujo y Sentencias */
    { "if",           FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_IF },
    { "else",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSE },
    { "elseif",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSEIF },
    { "while",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_WHILE },
    { "for",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOR },
    { "foreach",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOREACH },
    { "switch",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SWITCH },
    { "case",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CASE },
    { "default",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DEFAULT },
    { "break",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_BREAK },
    { "continue",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CONTINUE },
    { "return",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_RETURN },
    { "goto",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_GOTO },
    { "try",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TRY },
    { "catch",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CATCH },
    { "except",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXCEPT },
    { "final",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FINAL },

    /* POO, Estructuras y Módulos */
    { "class",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CLASS },
    { "struct",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_STRUCT },
    { "enum",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ENUM },
    { "from",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FROM },
    { "function",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FUNCTION },
    { "overrule",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_OVERRULE },
    { "include",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_INCLUDE },
    { "use",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_USE },
    { "export",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXPORT },
    { "super",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SUPER },
    { "self",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SELF },
    { "ancestorof",   FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ANCESTOROF },
    { "descendantof", FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DESCENDANTOF },
    { "typeof",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TYPEOF },
    { "true",         FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_KW_TRUE },
    { "false",        FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_KW_FALSE },

    /* Modificadores / Palabras Clave Tradicionales */
    { "private",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PRIVATE },
    { "protected",    FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PROTECTED },
    { "public",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PUBLIC },

    /* Modificadores Dinámicos de Acceso (Parentizados) */
    { "(private)",    FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PRIVATE },
    { "(protected)",  FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PROTECTED },
    { "(public)",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PUBLIC },
};

const size_t FOXY_KEYWORD_TABLE_SIZE = sizeof(FOXY_KEYWORD_TABLE) / sizeof(FOXY_KEYWORD_TABLE[0]);