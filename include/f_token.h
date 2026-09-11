#ifndef FOXY_TOKEN_H
#define FOXY_TOKEN_H

#include "f_settings.h" // configuraciones del lenguaje y del compilador. . .
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================= */
/* 1. CATEGORÍAS PRINCIPALES DE TOKENS (0-15) [4 bits]                       */
/* ========================================================================= */
#define FOXY_TOKEN_CAT_KEYWORD     1
#define FOXY_TOKEN_CAT_TYPE        2
#define FOXY_TOKEN_CAT_IDENTIFIER  3
#define FOXY_TOKEN_CAT_OPERATOR    4
#define FOXY_TOKEN_CAT_METHOD      5
#define FOXY_TOKEN_CAT_LITERAL     6
#define FOXY_TOKEN_CAT_ERROR       7

/* ========================================================================= */
/* 2. ENUMERADO GLOBAL DE TIPOS DE TOKEN (0-4095) [12 bits]                  */
/* ========================================================================= */
#if FOXY_COMPILER_SUPPORTS_XMACROS
#define FOXY_TOKEN_LIST(F) \
    /* Control y Errores */ \
    F(FOX_TOKEN_EOF) \
    F(FOX_TOKEN_ERROR) \
    /* Identificadores y Etiquetas */ \
    F(FOX_TOKEN_IDENTIFIER) \
    F(FOX_TOKEN_LABEL) \
    /* Literales */ \
    F(FOX_TOKEN_INT_LITERAL) \
    F(FOX_TOKEN_UINT_LITERAL) \
    F(FOX_TOKEN_LONG_LITERAL) \
    F(FOX_TOKEN_ULONG_LITERAL) \
    F(FOX_TOKEN_LLONG_LITERAL) \
    F(FOX_TOKEN_ULLONG_LITERAL) \
    F(FOX_TOKEN_FLOAT_LITERAL) \
    F(FOX_TOKEN_DOUBLE_LITERAL) \
    F(FOX_TOKEN_LDOUBLE_LITERAL) \
    F(FOX_TOKEN_NUMBER_LITERAL) \
    F(FOX_TOKEN_CHAR_LITERAL) \
    F(FOX_TOKEN_STRING_LITERAL) \
    /* Palabras Reservadas / Tipos de Datos Primitivos */ \
    F(FOX_TOKEN_KW_NULL) \
    F(FOX_TOKEN_KW_BOOL) \
    F(FOX_TOKEN_KW_CHAR) \
    F(FOX_TOKEN_KW_UCHAR) \
    F(FOX_TOKEN_KW_SHORT) \
    F(FOX_TOKEN_KW_USHORT) \
    F(FOX_TOKEN_KW_INT) \
    F(FOX_TOKEN_KW_UINT) \
    F(FOX_TOKEN_KW_LONG) \
    F(FOX_TOKEN_KW_ULONG) \
    F(FOX_TOKEN_KW_LLONG) \
    F(FOX_TOKEN_KW_ULLONG) \
    F(FOX_TOKEN_KW_FLOAT) \
    F(FOX_TOKEN_KW_DOUBLE) \
    F(FOX_TOKEN_KW_LDOUBLE) \
    F(FOX_TOKEN_KW_NUMBER) \
    F(FOX_TOKEN_KW_DICT) \
    F(FOX_TOKEN_KW_OBJECT) \
    /* Control de Flujo y Sentencias */ \
    F(FOX_TOKEN_KW_IF) \
    F(FOX_TOKEN_KW_ELSE) \
    F(FOX_TOKEN_KW_ELSEIF) \
    F(FOX_TOKEN_KW_WHILE) \
    F(FOX_TOKEN_KW_FOR) \
    F(FOX_TOKEN_KW_FOREACH) \
    F(FOX_TOKEN_KW_SWITCH) \
    F(FOX_TOKEN_KW_CASE) \
    F(FOX_TOKEN_KW_DEFAULT) \
    F(FOX_TOKEN_KW_BREAK) \
    F(FOX_TOKEN_KW_CONTINUE) \
    F(FOX_TOKEN_KW_RETURN) \
    F(FOX_TOKEN_KW_GOTO) \
    F(FOX_TOKEN_KW_TRY) \
    F(FOX_TOKEN_KW_CATCH) \
    F(FOX_TOKEN_KW_EXCEPT) \
    F(FOX_TOKEN_KW_FINAL) \
    /* POO, Estructuras y Módulos */ \
    F(FOX_TOKEN_KW_CLASS) \
    F(FOX_TOKEN_KW_STRUCT) \
    F(FOX_TOKEN_KW_ENUM) \
    F(FOX_TOKEN_KW_FROM) \
    F(FOX_TOKEN_KW_FUNCTION) \
    F(FOX_TOKEN_KW_OVERRULE) \
    F(FOX_TOKEN_KW_INCLUDE) \
    F(FOX_TOKEN_KW_USE) \
    F(FOX_TOKEN_KW_EXPORT) \
    F(FOX_TOKEN_KW_SUPER) \
    F(FOX_TOKEN_KW_SELF) \
    F(FOX_TOKEN_KW_ANCESTOROF) \
    F(FOX_TOKEN_KW_DESCENDANTOF) \
    F(FOX_TOKEN_KW_TYPEOF) \
    F(FOX_TOKEN_KW_TRUE) \
    F(FOX_TOKEN_KW_FALSE) \
    F(FOX_TOKEN_KW_PRIVATE) \
    F(FOX_TOKEN_KW_PROTECTED) \
    F(FOX_TOKEN_KW_PUBLIC) \
    /* Modificadores de Acceso */ \
    F(FOX_TOKEN_MOD_PRIVATE) \
    F(FOX_TOKEN_MOD_PROTECTED) \
    F(FOX_TOKEN_MOD_PUBLIC) \
    /* Operadores Aritméticos, Lógicos y Bitwise */ \
    F(FOX_TOKEN_PLUS) \
    F(FOX_TOKEN_MINUS) \
    F(FOX_TOKEN_STAR) \
    F(FOX_TOKEN_SLASH) \
    F(FOX_TOKEN_PERCENT) \
    F(FOX_TOKEN_POWER) \
    F(FOX_TOKEN_HASH) \
    F(FOX_TOKEN_ASSIGN) \
    F(FOX_TOKEN_PLUS_ASSIGN) \
    F(FOX_TOKEN_MINUS_ASSIGN) \
    F(FOX_TOKEN_STAR_ASSIGN) \
    F(FOX_TOKEN_SLASH_ASSIGN) \
    F(FOX_TOKEN_PERCENT_ASSIGN) \
    F(FOX_TOKEN_POWER_ASSIGN) \
    F(FOX_TOKEN_INC) \
    F(FOX_TOKEN_DEC) \
    F(FOX_TOKEN_EQ) \
    F(FOX_TOKEN_NEQ) \
    F(FOX_TOKEN_LT) \
    F(FOX_TOKEN_GT) \
    F(FOX_TOKEN_LE) \
    F(FOX_TOKEN_GE) \
    F(FOX_TOKEN_BANG) \
    F(FOX_TOKEN_AND) \
    F(FOX_TOKEN_OR) \
    F(FOX_TOKEN_AMPERSAND) \
    F(FOX_TOKEN_PIPE) \
    F(FOX_TOKEN_TILDE) \
    F(FOX_TOKEN_CARET) \
    F(FOX_TOKEN_LSHIFT) \
    F(FOX_TOKEN_RSHIFT) \
    /* Operadores Especiales y Delimitadores */ \
    F(FOX_TOKEN_ARROW) \
    F(FOX_TOKEN_PTR_ARROW) \
    F(FOX_TOKEN_ELLIPSIS) \
    F(FOX_TOKEN_LPAREN) \
    F(FOX_TOKEN_RPAREN) \
    F(FOX_TOKEN_LBRACE) \
    F(FOX_TOKEN_RBRACE) \
    F(FOX_TOKEN_LBRACKET) \
    F(FOX_TOKEN_RBRACKET) \
    F(FOX_TOKEN_SEMICOLON) \
    F(FOX_TOKEN_COLON) \
    F(FOX_TOKEN_COMMA) \
    F(FOX_TOKEN_DOT) \
    F(FOX_TOKEN_QUESTION)

#define F(ftoken) ftoken,
typedef enum __attribute__((__packed__)){
    FOXY_TOKEN_LIST(F)
} FoxyTokenType;
#undef F
#else
typedef enum {
    /* Control y Errores */
    FOX_TOKEN_EOF = 0,
    FOX_TOKEN_ERROR,

    /* Identificadores y Etiquetas */
    FOX_TOKEN_IDENTIFIER,
    FOX_TOKEN_LABEL,

    /* Literales */
    FOX_TOKEN_INT_LITERAL,
    FOX_TOKEN_UINT_LITERAL,
    FOX_TOKEN_LONG_LITERAL,
    FOX_TOKEN_ULONG_LITERAL,
    FOX_TOKEN_LLONG_LITERAL,
    FOX_TOKEN_ULLONG_LITERAL,
    FOX_TOKEN_FLOAT_LITERAL,
    FOX_TOKEN_DOUBLE_LITERAL,
    FOX_TOKEN_LDOUBLE_LITERAL,
    FOX_TOKEN_NUMBER_LITERAL,
    FOX_TOKEN_CHAR_LITERAL,
    FOX_TOKEN_STRING_LITERAL,

    /* Palabras Reservadas / Tipos de Datos Primitivos */
    FOX_TOKEN_KW_NULL,
    FOX_TOKEN_KW_BOOL,
    FOX_TOKEN_KW_CHAR,
    FOX_TOKEN_KW_UCHAR,
    FOX_TOKEN_KW_SHORT,
    FOX_TOKEN_KW_USHORT,
    FOX_TOKEN_KW_INT,
    FOX_TOKEN_KW_UINT,
    FOX_TOKEN_KW_LONG,
    FOX_TOKEN_KW_ULONG,
    FOX_TOKEN_KW_LLONG,
    FOX_TOKEN_KW_ULLONG,
    FOX_TOKEN_KW_FLOAT,
    FOX_TOKEN_KW_DOUBLE,
    FOX_TOKEN_KW_LDOUBLE,
    FOX_TOKEN_KW_NUMBER,
    FOX_TOKEN_KW_DICT,
    FOX_TOKEN_KW_OBJECT,

    /* Control de Flujo y Sentencias */
    FOX_TOKEN_KW_IF,
    FOX_TOKEN_KW_ELSE,
    FOX_TOKEN_KW_ELSEIF,
    FOX_TOKEN_KW_WHILE,
    FOX_TOKEN_KW_FOR,
    FOX_TOKEN_KW_FOREACH,
    FOX_TOKEN_KW_SWITCH,
    FOX_TOKEN_KW_CASE,
    FOX_TOKEN_KW_DEFAULT,
    FOX_TOKEN_KW_BREAK,
    FOX_TOKEN_KW_CONTINUE,
    FOX_TOKEN_KW_RETURN,
    FOX_TOKEN_KW_GOTO,
    FOX_TOKEN_KW_TRY,
    FOX_TOKEN_KW_CATCH,
    FOX_TOKEN_KW_EXCEPT,
    FOX_TOKEN_KW_FINAL,

    /* POO, Estructuras y Módulos */
    FOX_TOKEN_KW_CLASS,
    FOX_TOKEN_KW_STRUCT,
    FOX_TOKEN_KW_ENUM,
    FOX_TOKEN_KW_FROM,
    FOX_TOKEN_KW_FUNCTION,
    FOX_TOKEN_KW_OVERRULE,
    FOX_TOKEN_KW_INCLUDE,
    FOX_TOKEN_KW_USE,
    FOX_TOKEN_KW_EXPORT,
    FOX_TOKEN_KW_SUPER,
    FOX_TOKEN_KW_SELF,
    FOX_TOKEN_KW_ANCESTOROF,
    FOX_TOKEN_KW_DESCENDANTOF,
    FOX_TOKEN_KW_TYPEOF,
    FOX_TOKEN_KW_TRUE,
    FOX_TOKEN_KW_FALSE,
    FOX_TOKEN_KW_PRIVATE,
    FOX_TOKEN_KW_PROTECTED,
    FOX_TOKEN_KW_PUBLIC,

    /* Modificadores de Acceso */
    FOX_TOKEN_MOD_PRIVATE,
    FOX_TOKEN_MOD_PROTECTED,
    FOX_TOKEN_MOD_PUBLIC,

    /* Operadores Aritméticos, Lógicos y Bitwise */
    FOX_TOKEN_PLUS,
    FOX_TOKEN_MINUS,
    FOX_TOKEN_STAR,
    FOX_TOKEN_SLASH,
    FOX_TOKEN_PERCENT,
    FOX_TOKEN_POWER,
    FOX_TOKEN_HASH,
    FOX_TOKEN_ASSIGN,
    FOX_TOKEN_PLUS_ASSIGN,
    FOX_TOKEN_MINUS_ASSIGN,
    FOX_TOKEN_STAR_ASSIGN,
    FOX_TOKEN_SLASH_ASSIGN,
    FOX_TOKEN_PERCENT_ASSIGN,
    FOX_TOKEN_POWER_ASSIGN,
    FOX_TOKEN_INC,
    FOX_TOKEN_DEC,
    FOX_TOKEN_EQ,
    FOX_TOKEN_NEQ,
    FOX_TOKEN_LT,
    FOX_TOKEN_GT,
    FOX_TOKEN_LE,
    FOX_TOKEN_GE,
    FOX_TOKEN_BANG,
    FOX_TOKEN_AND,
    FOX_TOKEN_OR,
    FOX_TOKEN_AMPERSAND,
    FOX_TOKEN_PIPE,
    FOX_TOKEN_TILDE,
    FOX_TOKEN_CARET,
    FOX_TOKEN_LSHIFT,
    FOX_TOKEN_RSHIFT,

    /* Operadores Especiales y Delimitadores */
    FOX_TOKEN_ARROW,
    FOX_TOKEN_PTR_ARROW,
    FOX_TOKEN_ELLIPSIS,
    FOX_TOKEN_LPAREN,
    FOX_TOKEN_RPAREN,
    FOX_TOKEN_LBRACE,
    FOX_TOKEN_RBRACE,
    FOX_TOKEN_LBRACKET,
    FOX_TOKEN_RBRACKET,
    FOX_TOKEN_SEMICOLON,
    FOX_TOKEN_COLON,
    FOX_TOKEN_COMMA,
    FOX_TOKEN_DOT,
    FOX_TOKEN_QUESTION
} FoxyTokenType;
#endif

/* ========================================================================= */
/* 3. ESTRUCTURAS DE POSICIÓN Y TOKEN                                        */
/* ========================================================================= */

typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
} FoxySourcePos;

/**
 * @brief Token individual escaneado por el Lexer optimizado con campos de bits.
 */
typedef struct {
                        // no se puede mapear en campo de bits type a 12. . .
    FoxyTokenType type     : 7;  // Subtipo / FoxyTokenType (0-4095)
    const char *start;           // Puntero directo al texto del lexema
    uint32_t length;             // Longitud del lexema
    uint16_t type_category : 4;  // Categoría principal (1-15)
    FoxySourcePos pos;           // Ubicación exacta para reporte de errores
} FoxyToken;

typedef struct {
    const char *text;            // Texto reservado
    uint16_t category : 4;       // Categoría
    uint16_t subtype  : 12;      // Subtipo asignado
} FoxyKeywordMap;

extern const FoxyKeywordMap FOXY_KEYWORD_TABLE[];
extern const size_t FOXY_KEYWORD_TABLE_SIZE;

#endif // FOXY_TOKEN_H