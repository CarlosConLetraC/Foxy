#ifndef F_LEXER_H
#define F_LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "f_token.h"

/**
 * ============================================================================
 * FOXY LEXER TOKEN ENUMERATION
 * ============================================================================
 * Tokens soportados por el analizador léxico de Foxy.
 */
typedef enum {
    /* Token de control */
    FOX_TOKEN_EOF = 0,
    FOX_TOKEN_ERROR,

    /* Literales e Identificadores */
    FOX_TOKEN_IDENTIFIER,       // foo, Vector3, myLabel
    FOX_TOKEN_INT_LITERAL,      // 4i, 100, 32767
    FOX_TOKEN_UINT_LITERAL,     // 5ui, 20ui
    FOX_TOKEN_LONG_LITERAL,     // 6l, 99l
    FOX_TOKEN_ULONG_LITERAL,    // 7ul, 101ul
    FOX_TOKEN_LLONG_LITERAL,    // 8ll
    FOX_TOKEN_ULLONG_LITERAL,   // 9ull, 18446744073709551617
    FOX_TOKEN_FLOAT_LITERAL,    // 4.2f, 10f
    FOX_TOKEN_DOUBLE_LITERAL,   // 6.5, 11d, 2.718281
    FOX_TOKEN_LDOUBLE_LITERAL,  // 12ld, 1.414213562373095
    FOX_TOKEN_NUMBER_LITERAL,   // 13n (Literal de número dinámico)
    FOX_TOKEN_CHAR_LITERAL,     // 'a'
    FOX_TOKEN_STRING_LITERAL,   // "hola mundo"

    /* Palabras Reservadas - Tipos de Datos Primitivos */
    FOX_TOKEN_KW_NULL,          // null
    FOX_TOKEN_KW_BOOL,          // bool
    FOX_TOKEN_KW_CHAR,          // char
    FOX_TOKEN_KW_UCHAR,         // uchar
    FOX_TOKEN_KW_SHORT,         // short
    FOX_TOKEN_KW_USHORT,        // ushort
    FOX_TOKEN_KW_INT,           // int
    FOX_TOKEN_KW_UINT,          // uint
    FOX_TOKEN_KW_LONG,          // long
    FOX_TOKEN_KW_ULONG,         // ulong
    FOX_TOKEN_KW_LLONG,         // llong
    FOX_TOKEN_KW_ULLONG,        // ullong
    FOX_TOKEN_KW_FLOAT,         // float
    FOX_TOKEN_KW_DOUBLE,        // double
    FOX_TOKEN_KW_LDOUBLE,       // ldouble
    FOX_TOKEN_KW_NUMBER,        // number (Metatipo dinámico)
    FOX_TOKEN_KW_DICT,          // dict

    /* Palabras Reservadas - Control de Flujo y Sentencias */
    FOX_TOKEN_KW_IF,            // if
    FOX_TOKEN_KW_ELSE,          // else
    FOX_TOKEN_KW_ELSEIF,        // elseif
    FOX_TOKEN_KW_FOR,           // for
    FOX_TOKEN_KW_FOREACH,       // foreach
    FOX_TOKEN_KW_SWITCH,        // switch
    FOX_TOKEN_KW_CASE,          // case
    FOX_TOKEN_KW_DEFAULT,       // default
    FOX_TOKEN_KW_BREAK,         // break
    FOX_TOKEN_KW_CONTINUE,      // continue
    FOX_TOKEN_KW_RETURN,        // return
    FOX_TOKEN_KW_GOTO,          // goto
    FOX_TOKEN_KW_TRY,           // try
    FOX_TOKEN_KW_CATCH,         // catch
    FOX_TOKEN_KW_EXCEPT,        // except
    FOX_TOKEN_KW_FINAL,         // final

    /* Palabras Reservadas - POO, Estructuras y Módulos */
    FOX_TOKEN_KW_CLASS,         // class
    FOX_TOKEN_KW_STRUCT,        // struct
    FOX_TOKEN_KW_ENUM,          // enum
    FOX_TOKEN_KW_FROM,          // from (Herencia: class Cat from Animal)
    FOX_TOKEN_KW_FUNCTION,      // function
    FOX_TOKEN_KW_OVERRULE,      // overrule
    FOX_TOKEN_KW_INCLUDE,       // include
    FOX_TOKEN_KW_USE,           // use
    FOX_TOKEN_KW_EXPORT,        // export
    FOX_TOKEN_KW_SUPER,         // super
    FOX_TOKEN_KW_ANCESTOROF,    // ancestorof
    FOX_TOKEN_KW_DESCENDANTOF,  // descendantof
    FOX_TOKEN_KW_TYPEOF,        // typeof
    FOX_TOKEN_KW_TRUE,          // true
    FOX_TOKEN_KW_FALSE,         // false

    /* Modificadores de Acceso */
    FOX_TOKEN_MOD_PRIVATE,      // (private)
    FOX_TOKEN_MOD_PROTECTED,    // (protected)
    FOX_TOKEN_MOD_PUBLIC,       // (public)

    /* Operadores Aritméticos y Lógicos */
    FOX_TOKEN_PLUS,             // +
    FOX_TOKEN_MINUS,            // -
    FOX_TOKEN_STAR,             // *
    FOX_TOKEN_SLASH,            // /
    FOX_TOKEN_PERCENT,          // %
    FOX_TOKEN_POWER,            // **
    FOX_TOKEN_HASH,             // # (Operador de longitud)

    /* Operadores de Asignación */
    FOX_TOKEN_ASSIGN,           // =
    FOX_TOKEN_PLUS_ASSIGN,      // +=
    FOX_TOKEN_MINUS_ASSIGN,     // -=
    FOX_TOKEN_STAR_ASSIGN,      // *=
    FOX_TOKEN_SLASH_ASSIGN,     // /=
    FOX_TOKEN_PERCENT_ASSIGN,   // %=
    FOX_TOKEN_POWER_ASSIGN,     // **=

    /* Operadores Incrementales/Decrementales */
    FOX_TOKEN_INC,              // ++
    FOX_TOKEN_DEC,              // --

    /* Operadores de Relación y Comparación */
    FOX_TOKEN_EQ,               // ==
    FOX_TOKEN_NEQ,              // !=
    FOX_TOKEN_LT,               // <
    FOX_TOKEN_GT,               // >
    FOX_TOKEN_LE,               // <=
    FOX_TOKEN_GE,               // >=

    /* Operadores Lógicos y Bitwise */
    FOX_TOKEN_BANG,             // !
    FOX_TOKEN_AND,              // &&
    FOX_TOKEN_OR,               // ||
    FOX_TOKEN_AMPERSAND,        // &
    FOX_TOKEN_PIPE,             // |
    FOX_TOKEN_TILDE,            // ~
    FOX_TOKEN_CARET,            // ^
    FOX_TOKEN_LSHIFT,           // <<
    FOX_TOKEN_RSHIFT,           // >>

    /* Operadores Especiales y Símbolos */
    FOX_TOKEN_ARROW,            // => (Expresiones Lambda)
    FOX_TOKEN_PTR_ARROW,        // -> (Acceso a miembros de struct)
    FOX_TOKEN_ELLIPSIS,         // ... (Variádico: nums...)
    FOX_TOKEN_LABEL,            // <label_name> (Etiquetas Goto)

    /* Delimitadores */
    FOX_TOKEN_LPAREN,           // (
    FOX_TOKEN_RPAREN,           // )
    FOX_TOKEN_LBRACE,           // {
    FOX_TOKEN_RBRACE,           // }
    FOX_TOKEN_LBRACKET,         // [
    FOX_TOKEN_RBRACKET,         // ]
    FOX_TOKEN_SEMICOLON,        // ;
    FOX_TOKEN_COLON,            // :
    FOX_TOKEN_COMMA,            // ,
    FOX_TOKEN_DOT,              // .
    FOX_TOKEN_QUESTION          // ?
} FoxyTokenType;

/**
 * @brief Estructura de información de posición en código fuente.
 */
typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
} FoxySourcePos;

/**
* @brief Estado interno del analizador léxico.
*/
typedef struct {
    // FoxyTokenType type;
    const char *start;          // Puntero al inicio de la cadena en el buffer
    uint32_t length;            // Longitud del lexema
    FoxySourcePos pos;          // Posición exacta para reporte de errores
    const char *source;
    const char *cursor;
    const char *token_start;
    const char *filename;
    uint32_t line;
    uint32_t column;
} FoxyLexer;

/* API Principal del Lexer */
void f_lexer_init(FoxyLexer *lexer, const char *source, const char *filename);
FoxyToken f_lexer_next_token(FoxyLexer *lexer);
FoxyToken f_lexer_peek_token(FoxyLexer *lexer);

/* Helpers y Diagnóstico */
const char *f_lexer_token_type_to_string(FoxyTokenType type);
void f_lexer_print_token(const FoxyToken *token);

#endif // F_LEXER_HD