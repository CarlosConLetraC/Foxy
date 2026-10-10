#pragma once

#include "f_settings.h"
#include "f_foxcode.h"
#include "f_foxmode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

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
/* 2. ENUMERADO GLOBAL DE TIPOS DE TOKEN EN CRUDO                             */
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
    /* Palabras clave de modificadores / especificadores */ \
    F(FOX_TOKEN_KW_GLOBAL) \
    F(FOX_TOKEN_KW_STATIC) \
    F(FOX_TOKEN_KW_CONST) \
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
    F(FOX_TOKEN_KW_PARENTOF) \
    F(FOX_TOKEN_KW_CHILDOF) \
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
    /* Métodos Marcados / Flagged Methods */ \
    F(FOX_TOKEN_METHOD_NEW) \
    F(FOX_TOKEN_METHOD_CAST) \
    F(FOX_TOKEN_METHOD_TOSTRING) \
    F(FOX_TOKEN_METHOD_ADD) \
    F(FOX_TOKEN_METHOD_SUB) \
    F(FOX_TOKEN_METHOD_MUL) \
    F(FOX_TOKEN_METHOD_DIV) \
    F(FOX_TOKEN_METHOD_POW) \
    F(FOX_TOKEN_METHOD_MOD) \
    F(FOX_TOKEN_METHOD_CONCAT) \
    F(FOX_TOKEN_METHOD_UNM) \
    F(FOX_TOKEN_METHOD_NOT) \
    F(FOX_TOKEN_METHOD_EQ) \
    F(FOX_TOKEN_METHOD_NEQ) \
    F(FOX_TOKEN_METHOD_LT) \
    F(FOX_TOKEN_METHOD_GT) \
    F(FOX_TOKEN_METHOD_LE) \
    F(FOX_TOKEN_METHOD_GE) \
    F(FOX_TOKEN_METHOD_BAND) \
    F(FOX_TOKEN_METHOD_BOR) \
    F(FOX_TOKEN_METHOD_BNOT) \
    F(FOX_TOKEN_METHOD_BXOR) \
    F(FOX_TOKEN_METHOD_LSHIFT) \
    F(FOX_TOKEN_METHOD_RSHIFT) \
    F(FOX_TOKEN_METHOD_FOREACH) \
    F(FOX_TOKEN_METHOD_CLOSED) \
    F(FOX_TOKEN_METHOD_LEN) \
    F(FOX_TOKEN_METHOD_UNPACK) \
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
    F(FOX_TOKEN_AND_ASSIGN) \
    F(FOX_TOKEN_OR_ASSIGN) \
    F(FOX_TOKEN_XOR_ASSIGN) \
    F(FOX_TOKEN_LSHIFT_ASSIGN) \
    F(FOX_TOKEN_RSHIFT_ASSIGN) \
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
    F(FOX_TOKEN_FAT_ARROW) \
    F(FOX_TOKEN_PTR_ARROW) \
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
    F(FOX_TOKEN_DOTDOT) \
    F(FOX_TOKEN_DOTDOTDOT) \
    F(FOX_TOKEN_QUESTION)

#define F(ftoken) ftoken,
typedef enum FOXY_PACKED {
    FOXY_TOKEN_LIST(F)
} FoxyTokenType;
#undef F
#else
typedef enum FOXY_PACKED {
    FOX_TOKEN_EOF = 0,
    FOX_TOKEN_ERROR,
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
    /* Palabras clave */
    FOX_TOKEN_KW_GLOBAL,
    FOX_TOKEN_KW_STATIC,
    FOX_TOKEN_KW_CONST,
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
    /* Control de Flujo */
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
    /* POO */
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
    FOX_TOKEN_KW_PARENTOF,
    FOX_TOKEN_KW_CHILDOF,
    FOX_TOKEN_KW_TYPEOF,
    FOX_TOKEN_KW_TRUE,
    FOX_TOKEN_KW_FALSE,
    FOX_TOKEN_KW_PRIVATE,
    FOX_TOKEN_KW_PROTECTED,
    FOX_TOKEN_KW_PUBLIC,
    FOX_TOKEN_MOD_PRIVATE,
    FOX_TOKEN_MOD_PROTECTED,
    FOX_TOKEN_MOD_PUBLIC,
    /* Métodos Marcados */
    FOX_TOKEN_METHOD_NEW,
    FOX_TOKEN_METHOD_CAST,
    FOX_TOKEN_METHOD_TOSTRING,
    FOX_TOKEN_METHOD_ADD,
    FOX_TOKEN_METHOD_SUB,
    FOX_TOKEN_METHOD_MUL,
    FOX_TOKEN_METHOD_DIV,
    FOX_TOKEN_METHOD_POW,
    FOX_TOKEN_METHOD_MOD,
    FOX_TOKEN_METHOD_CONCAT,
    FOX_TOKEN_METHOD_UNM,
    FOX_TOKEN_METHOD_NOT,
    FOX_TOKEN_METHOD_EQ,
    FOX_TOKEN_METHOD_NEQ,
    FOX_TOKEN_METHOD_LT,
    FOX_TOKEN_METHOD_GT,
    FOX_TOKEN_METHOD_LE,
    FOX_TOKEN_METHOD_GE,
    FOX_TOKEN_METHOD_BAND,
    FOX_TOKEN_METHOD_BOR,
    FOX_TOKEN_METHOD_BNOT,
    FOX_TOKEN_METHOD_BXOR,
    FOX_TOKEN_METHOD_LSHIFT,
    FOX_TOKEN_METHOD_RSHIFT,
    FOX_TOKEN_METHOD_FOREACH,
    FOX_TOKEN_METHOD_CLOSED,
    FOX_TOKEN_METHOD_LEN,
    FOX_TOKEN_METHOD_UNPACK,
    /* Operadores y Delimitadores */
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
    FOX_TOKEN_AND_ASSIGN,
    FOX_TOKEN_OR_ASSIGN,
    FOX_TOKEN_XOR_ASSIGN,
    FOX_TOKEN_LSHIFT_ASSIGN,
    FOX_TOKEN_RSHIFT_ASSIGN,
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
    FOX_TOKEN_FAT_ARROW,
    FOX_TOKEN_PTR_ARROW,
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
    FOX_TOKEN_DOTDOT,
    FOX_TOKEN_DOTDOTDOT,
    FOX_TOKEN_QUESTION
} FoxyTokenType;
#endif

/* ========================================================================= */
/* 3. ESTRUCTURAS Y COMPOSICIÓN BITWISE DEL LEXER                             */
/* ========================================================================= */

static inline uint16_t f_lexer_compose_type(uint16_t category, FoxyTokenType subtype) {
    return f_foxmode_compose_type(category, (uint16_t)subtype);
}

typedef struct {
    const char *filename;  /* 8 bytes */
    uint32_t line;         /* 4 bytes */
    uint32_t column;       /* 4 bytes */
} FoxySourcePos;           /* 8 + 4 + 4 = 16 bytes */

typedef struct {
    const char *start;       /* 8 bytes */
    FoxySourcePos pos;       /* 16 bytes */
    uint32_t length;         /* 4 bytes */
    uint16_t flags;          /* 2 bytes (reemplaza el padding con banderas de lexer) */
    FoxyTokenType type;      /* 1 byte */
    uint8_t type_category;   /* 1 byte */
} FoxyToken;                 /* Exactamente 32 bytes sin desperdicio */

typedef struct {
    const char *text;      /* 8 bytes [offset 0..7] */
    uint16_t category;     /* 2 bytes [offset 8..9] */
    uint16_t subtype;      /* 2 bytes [offset 10..11] */
    uint32_t flags;        /* 4 bytes útiles en lugar de padding [offset 12..15] */
} FoxyKeywordMap;          /* Exactamente 16 bytes sin padding residual */

extern const FoxyKeywordMap FOXY_KEYWORD_TABLE[];
extern const size_t FOXY_KEYWORD_TABLE_SIZE;

typedef struct {
    FILE *file;               /* 8 bytes: Handle de archivo si se leyó de disco */
    const char *filename;     /* 8 bytes: Nombre del archivo para reporte de errores */
    const char *source;       /* 8 bytes: Apunta al inicio del buffer completo en memoria */
    const char *cursor;       /* 8 bytes: Posición actual dentro del fuente */
    const char *token_start;  /* 8 bytes: Inicio del lexema actual */
    uint32_t line;            /* 4 bytes: Fila actual */
    uint32_t column;          /* 4 bytes: Columna actual */
    uint8_t flags;            /* 1 byte  */
    uint8_t is_eof;           /* 1 byte  */
    uint8_t owns_source;      /* 1 byte: Indica si el lexer debe hacer free(source) */
    uint8_t reserved[5];      /* 5 bytes: Padding explícito (Estructura total = 56 bytes) */
} FoxyLexer;

/* ========================================================================= */
/* 4. FUNCIONES PÚBLICAS                                                    */
/* ========================================================================= */
FOXY_EXPORT bool f_lexer_init_file(FoxyLexer *lexer, FILE *file, const char *filename);
FOXY_EXPORT bool f_lexer_init_string(FoxyLexer *lexer, const char *source, const char *filename);
FOXY_EXPORT void f_lexer_close(FoxyLexer *lexer);
FOXY_EXPORT FoxyToken f_lexer_next_token(FoxyLexer *lexer);
FOXY_EXPORT FoxyToken f_lexer_peek_token(FoxyLexer *lexer);
FOXY_EXPORT const char *f_lexer_token_type_to_string(FoxyTokenType type);
FOXY_EXPORT void f_lexer_print_token(const FoxyToken *token);
FOXY_EXPORT void f_lexer_error(FoxyLexer *lexer, const char *format, ...);

/* ========================================================================= */
/* 5. MÁSCARAS BITWISE MULTIPALABRA PARA TOKENS (LEXER / PARSER)              */
/* ========================================================================= */

/* --- Primary / Literales & Identificadores --- */
#define FOXY_MASK_PRIMARY_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOX_TOKEN_IDENTIFIER)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_INT_LITERAL)     | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_UINT_LITERAL)    | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_LONG_LITERAL)    | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_ULONG_LITERAL)   | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_LLONG_LITERAL)   | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_ULLONG_LITERAL)  | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_FLOAT_LITERAL)   | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_DOUBLE_LITERAL)  | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_LDOUBLE_LITERAL) | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_NUMBER_LITERAL)  | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_CHAR_LITERAL)    | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_STRING_LITERAL)  | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_NULL),          \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_TRUE)         | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_FALSE),        \
    /* Word 2 [128..191] */ 0ULL,                \
    /* Word 3 [192..255] */ 0ULL                 \
)

/* --- Postfix (Op. Postfijos: Acceso, Llamada, Incrementos) --- */
#define FOXY_MASK_POSTFIX_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                   \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_INC)             | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_DEC),              \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LPAREN)          | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LBRACKET)        | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_DOT),              \
    /* Word 3 [192..255] */ 0ULL                 \
)

/* --- Unary (Op. Unarios Pre-fijos) --- */
#define FOXY_MASK_UNARY_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                  \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MINUS)           | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_INC)             | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_DEC),              \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_BANG)            | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_TILDE),            \
    /* Word 3 [192..255] */ 0ULL                 \
)

/* --- Multiplicative (*, /, %) --- */
#define FOXY_MASK_MULTIPLICATIVE_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                        \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_STAR)            | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_SLASH)           | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PERCENT),          \
    /* Word 2 [128..191] */ 0ULL,                     \
    /* Word 3 [192..255] */ 0ULL                      \
)

/* --- Additive (+, -) --- */
#define FOXY_MASK_ADDITIVE_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                    \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PLUS)            | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MINUS),            \
    /* Word 2 [128..191] */ 0ULL,                 \
    /* Word 3 [192..255] */ 0ULL                  \
)

/* --- Relational (<, <=, >, >=) --- */
#define FOXY_MASK_RELATIONAL_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                      \
    /* Word 1 [64..127] */ 0ULL,                    \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LT)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LE)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_GT)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_GE),               \
    /* Word 3 [192..255] */ 0ULL                    \
)

/* --- Equality (==, !=) --- */
#define FOXY_MASK_EQUALITY_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                    \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_EQ),               \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_NEQ),              \
    /* Word 3 [192..255] */ 0ULL                  \
)

/* --- Logical (&&, ||) --- */
#define FOXY_MASK_LOGICAL_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                   \
    /* Word 1 [64..127] */ 0ULL,                 \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_AND)             | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_OR),               \
    /* Word 3 [192..255] */ 0ULL                 \
)

/* --- Assignment (=, +=, -=, *=, /=, %=, etc.) --- */
#define FOXY_MASK_ASSIGNMENT_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL,                      \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_ASSIGN)          | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PLUS_ASSIGN)     | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MINUS_ASSIGN)    | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_STAR_ASSIGN)     | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_SLASH_ASSIGN)    | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PERCENT_ASSIGN)  | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_POWER_ASSIGN)    | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_AND_ASSIGN)      | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_OR_ASSIGN)       | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_XOR_ASSIGN)      | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_LSHIFT_ASSIGN)   | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_RSHIFT_ASSIGN),    \
    /* Word 2 [128..191] */ 0ULL,                   \
    /* Word 3 [192..255] */ 0ULL                    \
)

/* --- Expresiones Binarias --- */
#define FOXY_MASK_BINARY_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL, \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PLUS)            | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MINUS)           | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_STAR)            | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_SLASH)           | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_PERCENT)         | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_POWER)           | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_EQ),               \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_NEQ)             | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LT)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_GT)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LE)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_GE)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_AND)             | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_OR)              | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_AMPERSAND)       | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_PIPE)            | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_CARET)           | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_LSHIFT)          | \
    FOXY_TOK_BIT_W2(FOX_TOKEN_RSHIFT),           \
    /* Word 3 [192..255] */ 0ULL  \
)

/* --- Inicios de Sentencias (Statement Start) --- */
#define FOXY_MASK_STMT_START_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_CLASS)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_STRUCT)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_ENUM)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FUNCTION)     | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FOR)          | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FOREACH)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_IF)           | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_WHILE)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_RETURN)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_SWITCH)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_TRY)          | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_INCLUDE),      \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/* --- Sincronización de Errores (Parser Recovery) --- */
#define FOXY_MASK_SYNC_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_IF)           | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_WHILE)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FOR)          | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FOREACH)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_SWITCH)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_RETURN)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_TRY)          | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_CLASS)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_STRUCT)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FUNCTION),      \
    /* Word 1 [64..127] */ 0ULL,                     \
    /* Word 2 [128..191] */ \
    FOXY_TOK_BIT_W2(FOX_TOKEN_SEMICOLON),        \
    /* Word 3 [192..255] */ 0ULL                     \
)

/* --- Especificadores de Tipos --- */
#define FOXY_MASK_TYPE_SPECIFIERS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_BOOL)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_CHAR)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_UCHAR)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_SHORT)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_USHORT)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_INT)          | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_UINT)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_LONG)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_ULONG)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_LLONG)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_ULLONG)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_FLOAT)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_DOUBLE)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_LDOUBLE)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_NUMBER)       | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_DICT)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_OBJECT)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_STRUCT)        | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_CLASS)         | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_ENUM),          \
    /* Word 1 [64..127]  */ 0ULL, \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/* --- Tokens Declarativos (Modificadores de Visibilidad) --- */
#define FOXY_MASK_DECLARATIVE_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ 0ULL, \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PRIVATE)      | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PROTECTED)    | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PUBLIC)       | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PRIVATE)     | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PROTECTED)   | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PUBLIC),       \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/* --- Calificadores de Variables --- */
#define FOXY_MASK_QUALIFIER_TOKENS FOXY_MAKE_MASK( \
    /* Word 0 [0..63] */ \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_GLOBAL)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_STATIC)      | \
    FOXY_TOK_BIT_W0(FOX_TOKEN_KW_CONST),        \
    /* Word 1 [64..127] */ \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_EXPORT)      | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PRIVATE)     | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PROTECTED)   | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_KW_PUBLIC)      | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PRIVATE)    | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PROTECTED)  | \
    FOXY_TOK_BIT_W1(FOX_TOKEN_MOD_PUBLIC),      \
    /* Word 2 [128..191] */ 0ULL, \
    /* Word 3 [192..255] */ 0ULL  \
)

/* Función inline de verificación delegada a foxy_mask_contains */
static inline bool f_token_is_in_mask(uint32_t token_type, foxy_mask_t mask) {
    return foxy_mask_contains(mask, token_type);
}

/* Predicados de conveniencia */
static inline bool f_token_is_primary(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_PRIMARY_TOKENS);
}

static inline bool f_token_is_postfix(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_POSTFIX_TOKENS);
}

static inline bool f_token_is_unary(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_UNARY_TOKENS);
}

static inline bool f_token_is_multiplicative(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_MULTIPLICATIVE_TOKENS);
}

static inline bool f_token_is_additive(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_ADDITIVE_TOKENS);
}

static inline bool f_token_is_relational(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_RELATIONAL_TOKENS);
}

static inline bool f_token_is_equality(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_EQUALITY_TOKENS);
}

static inline bool f_token_is_logical(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_LOGICAL_TOKENS);
}

static inline bool f_token_is_assignment(uint32_t type) {
    return f_token_is_in_mask(type, FOXY_MASK_ASSIGNMENT_TOKENS);
}

/**
 * @brief Evalúa si un token es un punto de límite/inicio válido para descartar e interactuar con panic mode.
 */
static inline bool f_token_is_sync_boundary(uint32_t token_type) {
    return f_token_is_in_mask(token_type, FOXY_MASK_SYNC_TOKENS);
}

/**
 * @brief Evalúa si un token pertenece a un tipo de especificador de tipo.
 */
static inline bool f_token_is_type_specifier(FoxyTokenType type) {
    return f_token_is_in_mask((uint32_t)type, FOXY_MASK_TYPE_SPECIFIERS);
}

typedef struct {
    const char *suffix;       /* Texto del sufijo (ej. "ui", "ld", "n") (8 bytes) */
    FoxyTokenType token_type; /* Tipo de token resultante (4 bytes con FOXY_PACKED) */
    uint8_t length;           /* Longitud del sufijo para comparación rápida (1 byte) */
    uint8_t _pad[6];
} FoxySuffixRule;

static const FoxySuffixRule FOXY_SUFFIX_TABLE[] = {
    { .suffix = "i",   .token_type = FOX_TOKEN_INT_LITERAL,     .length = 1 },
    { .suffix = "ui",  .token_type = FOX_TOKEN_UINT_LITERAL,    .length = 2 },
    { .suffix = "l",   .token_type = FOX_TOKEN_LONG_LITERAL,    .length = 1 },
    { .suffix = "ul",  .token_type = FOX_TOKEN_ULONG_LITERAL,   .length = 2 },
    { .suffix = "ll",  .token_type = FOX_TOKEN_LLONG_LITERAL,   .length = 2 },
    { .suffix = "ull", .token_type = FOX_TOKEN_ULLONG_LITERAL,  .length = 3 },
    { .suffix = "f",   .token_type = FOX_TOKEN_FLOAT_LITERAL,   .length = 1 },
    { .suffix = "d",   .token_type = FOX_TOKEN_DOUBLE_LITERAL,  .length = 1 },
    { .suffix = "ld",  .token_type = FOX_TOKEN_LDOUBLE_LITERAL, .length = 2 },
    { .suffix = "n",   .token_type = FOX_TOKEN_NUMBER_LITERAL,  .length = 1 }
};

static const size_t FOXY_SUFFIX_TABLE_SIZE = sizeof(FOXY_SUFFIX_TABLE) / sizeof(FoxySuffixRule);