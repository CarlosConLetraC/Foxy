#include "f_lexer.h"
#include "f_exceptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* ========================================================================= */
/* TABLA DE NOMBRES DE TOKENS (PARA IMPRESIÓN Y DEBUG)                        */
/* ========================================================================= */
#if FOXY_COMPILER_SUPPORTS_XMACROS
static const char *const FOXY_TOKEN_NAMES[] = {
    #define F(ftoken) #ftoken,
    FOXY_TOKEN_LIST(F)
    #undef F
};
#else
static const char *const FOXY_TOKEN_NAMES[] = {
    /* Control y Errores */
    "FOX_TOKEN_EOF",
    "FOX_TOKEN_ERROR",
    /* Identificadores y Etiquetas */
    "FOX_TOKEN_IDENTIFIER",
    "FOX_TOKEN_CUSTOM_DATATYPE_IDENTIFIER",
    "FOX_TOKEN_LABEL",
    /* Literales */
    "FOX_TOKEN_INT_LITERAL",
    "FOX_TOKEN_UINT_LITERAL",
    "FOX_TOKEN_LONG_LITERAL",
    "FOX_TOKEN_ULONG_LITERAL",
    "FOX_TOKEN_LLONG_LITERAL",
    "FOX_TOKEN_ULLONG_LITERAL",
    "FOX_TOKEN_FLOAT_LITERAL",
    "FOX_TOKEN_DOUBLE_LITERAL",
    "FOX_TOKEN_LDOUBLE_LITERAL",
    "FOX_TOKEN_NUMBER_LITERAL",
    "FOX_TOKEN_CHAR_LITERAL",
    "FOX_TOKEN_STRING_LITERAL",
    /* Palabras clave de modificadores / especificadores */
    "FOX_TOKEN_KW_GLOBAL",
    "FOX_TOKEN_KW_STATIC",
    "FOX_TOKEN_KW_CONST",
    /* Palabras Reservadas / Tipos de Datos Primitivos */
    "FOX_TOKEN_KW_CUSTOM_DATATYPE",
    "FOX_TOKEN_KW_NULL",
    "FOX_TOKEN_KW_BOOL",
    "FOX_TOKEN_KW_CHAR",
    "FOX_TOKEN_KW_UCHAR",
    "FOX_TOKEN_KW_SHORT",
    "FOX_TOKEN_KW_USHORT",
    "FOX_TOKEN_KW_INT",
    "FOX_TOKEN_KW_UINT",
    "FOX_TOKEN_KW_LONG",
    "FOX_TOKEN_KW_ULONG",
    "FOX_TOKEN_KW_LLONG",
    "FOX_TOKEN_KW_ULLONG",
    "FOX_TOKEN_KW_FLOAT",
    "FOX_TOKEN_KW_DOUBLE",
    "FOX_TOKEN_KW_LDOUBLE",
    "FOX_TOKEN_KW_NUMBER",
    "FOX_TOKEN_KW_DICT",
    /* Control de Flujo y Sentencias */
    "FOX_TOKEN_KW_IF",
    "FOX_TOKEN_KW_ELSE",
    "FOX_TOKEN_KW_ELSEIF",
    "FOX_TOKEN_KW_WHILE",
    "FOX_TOKEN_KW_FOR",
    "FOX_TOKEN_KW_FOREACH",
    "FOX_TOKEN_KW_SWITCH",
    "FOX_TOKEN_KW_CASE",
    "FOX_TOKEN_KW_DEFAULT",
    "FOX_TOKEN_KW_BREAK",
    "FOX_TOKEN_KW_CONTINUE",
    "FOX_TOKEN_KW_RETURN",
    "FOX_TOKEN_KW_GOTO",
    "FOX_TOKEN_KW_TRY",
    "FOX_TOKEN_KW_CATCH",
    "FOX_TOKEN_KW_EXCEPT",
    "FOX_TOKEN_KW_FINAL",
    /* POO, Estructuras y Módulos */
    "FOX_TOKEN_KW_CLASS",
    "FOX_TOKEN_KW_STRUCT",
    "FOX_TOKEN_KW_ENUM",
    "FOX_TOKEN_KW_FROM",
    "FOX_TOKEN_KW_FUNCTION",
    "FOX_TOKEN_KW_OVERRULE",
    "FOX_TOKEN_KW_INCLUDE",
    "FOX_TOKEN_KW_USE",
    "FOX_TOKEN_KW_EXPORT",
    "FOX_TOKEN_KW_SUPER",
    "FOX_TOKEN_KW_SELF",
    "FOX_TOKEN_KW_ANCESTOROF",
    "FOX_TOKEN_KW_DESCENDANTOF",
    "FOX_TOKEN_KW_PARENTOF",
    "FOX_TOKEN_KW_CHILDOF",
    "FOX_TOKEN_KW_TYPEOF",
    "FOX_TOKEN_KW_TRUE",
    "FOX_TOKEN_KW_FALSE",
    "FOX_TOKEN_KW_PRIVATE",
    "FOX_TOKEN_KW_PROTECTED",
    "FOX_TOKEN_KW_PUBLIC",
    /* Modificadores de Acceso */
    "FOX_TOKEN_MOD_PRIVATE",
    "FOX_TOKEN_MOD_PROTECTED",
    "FOX_TOKEN_MOD_PUBLIC",
    /* Métodos Marcados / Flagged Methods */
    "FOX_TOKEN_METHOD_NEW",
    "FOX_TOKEN_METHOD_CAST",
    "FOX_TOKEN_METHOD_TOSTRING",
    "FOX_TOKEN_METHOD_ADD",
    "FOX_TOKEN_METHOD_SUB",
    "FOX_TOKEN_METHOD_MUL",
    "FOX_TOKEN_METHOD_DIV",
    "FOX_TOKEN_METHOD_POW",
    "FOX_TOKEN_METHOD_MOD",
    "FOX_TOKEN_METHOD_CONCAT",
    "FOX_TOKEN_METHOD_UNM",
    "FOX_TOKEN_METHOD_NOT",
    "FOX_TOKEN_METHOD_EQ",
    "FOX_TOKEN_METHOD_NEQ",
    "FOX_TOKEN_METHOD_LT",
    "FOX_TOKEN_METHOD_GT",
    "FOX_TOKEN_METHOD_LE",
    "FOX_TOKEN_METHOD_GE",
    "FOX_TOKEN_METHOD_BAND",
    "FOX_TOKEN_METHOD_BOR",
    "FOX_TOKEN_METHOD_BNOT",
    "FOX_TOKEN_METHOD_BXOR",
    "FOX_TOKEN_METHOD_LSHIFT",
    "FOX_TOKEN_METHOD_RSHIFT",
    "FOX_TOKEN_METHOD_FOREACH",
    "FOX_TOKEN_METHOD_CLOSED",
    "FOX_TOKEN_METHOD_LEN",
    "FOX_TOKEN_METHOD_UNPACK",
    /* Operadores Aritméticos, Lógicos y Bitwise */
    "FOX_TOKEN_PLUS",
    "FOX_TOKEN_MINUS",
    "FOX_TOKEN_STAR",
    "FOX_TOKEN_SLASH",
    "FOX_TOKEN_PERCENT",
    "FOX_TOKEN_POWER",
    "FOX_TOKEN_HASH",
    "FOX_TOKEN_ASSIGN",
    "FOX_TOKEN_PLUS_ASSIGN",
    "FOX_TOKEN_MINUS_ASSIGN",
    "FOX_TOKEN_STAR_ASSIGN",
    "FOX_TOKEN_SLASH_ASSIGN",
    "FOX_TOKEN_PERCENT_ASSIGN",
    "FOX_TOKEN_POWER_ASSIGN",
    "FOX_TOKEN_AND_ASSIGN",
    "FOX_TOKEN_OR_ASSIGN",
    "FOX_TOKEN_XOR_ASSIGN",
    "FOX_TOKEN_LSHIFT_ASSIGN",
    "FOX_TOKEN_RSHIFT_ASSIGN",
    "FOX_TOKEN_INC",
    "FOX_TOKEN_DEC",
    "FOX_TOKEN_EQ",
    "FOX_TOKEN_NEQ",
    "FOX_TOKEN_LT",
    "FOX_TOKEN_GT",
    "FOX_TOKEN_LE",
    "FOX_TOKEN_GE",
    "FOX_TOKEN_BANG",
    "FOX_TOKEN_AND",
    "FOX_TOKEN_OR",
    "FOX_TOKEN_AMPERSAND",
    "FOX_TOKEN_PIPE",
    "FOX_TOKEN_TILDE",
    "FOX_TOKEN_CARET",
    "FOX_TOKEN_LSHIFT",
    "FOX_TOKEN_RSHIFT",
    /* Operadores Especiales y Delimitadores */
    "FOX_TOKEN_FAT_ARROW",
    "FOX_TOKEN_PTR_ARROW",
    "FOX_TOKEN_LPAREN",
    "FOX_TOKEN_RPAREN",
    "FOX_TOKEN_LBRACE",
    "FOX_TOKEN_RBRACE",
    "FOX_TOKEN_LBRACKET",
    "FOX_TOKEN_RBRACKET",
    "FOX_TOKEN_SEMICOLON",
    "FOX_TOKEN_COLON",
    "FOX_TOKEN_COMMA",
    "FOX_TOKEN_DOT",
    "FOX_TOKEN_DOTDOT",
    "FOX_TOKEN_DOTDOTDOT",
    "FOX_TOKEN_QUESTION"
};
#endif

/* ========================================================================= */
/* TABLA DE PALABRAS CLAVE RESERVADAS                                        */
/* ========================================================================= */
const FoxyKeywordMap FOXY_KEYWORD_TABLE[] = {
    /* Modificadores / Especificadores */
    {.text = "global",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_GLOBAL,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "static",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_STATIC,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "const",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CONST,        .flags = FOXY_KW_FLAG_NONE},

    /* Palabras Reservadas / Primitivos */
    {.text = "null",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_NULL,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "bool",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_BOOL,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "char",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_CHAR,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "uchar",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_UCHAR,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "short",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_SHORT,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "ushort",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_USHORT,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "int",          .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_INT,          .flags = FOXY_KW_FLAG_NONE},
    {.text = "uint",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_UINT,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "long",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LONG,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "ulong",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_ULONG,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "llong",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LLONG,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "ullong",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_ULLONG,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "float",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_FLOAT,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "double",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_DOUBLE,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "ldOUBLE",      .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LDOUBLE,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "number",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_NUMBER,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "dict",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_DICT,         .flags = FOXY_KW_FLAG_NONE},

    /* Control de Flujo */
    {.text = "if",           .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_IF,           .flags = FOXY_KW_FLAG_NONE},
    {.text = "else",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ELSE,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "elseif",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ELSEIF,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "while",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_WHILE,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "for",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FOR,          .flags = FOXY_KW_FLAG_NONE},
    {.text = "foreach",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FOREACH,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "switch",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SWITCH,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "case",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CASE,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "default",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_DEFAULT,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "break",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_BREAK,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "continue",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CONTINUE,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "return",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_RETURN,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "goto",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_GOTO,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "try",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_TRY,          .flags = FOXY_KW_FLAG_NONE},
    {.text = "catch",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CATCH,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "except",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_EXCEPT,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "final",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FINAL,        .flags = FOXY_KW_FLAG_NONE},

    /* POO y Estructuras */
    {.text = "class",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CLASS,        .flags = FOXY_KW_FLAG_CUSTOM_TYPE},
    {.text = "struct",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_STRUCT,       .flags = FOXY_KW_FLAG_CUSTOM_TYPE},
    {.text = "enum",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ENUM,         .flags = FOXY_KW_FLAG_CUSTOM_TYPE},
    {.text = "from",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FROM,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "function",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FUNCTION,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "overrule",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_OVERRULE,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "include",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_INCLUDE,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "use",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_USE,          .flags = FOXY_KW_FLAG_NONE},
    {.text = "export",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_EXPORT,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "super",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SUPER,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "self",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SELF,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "ancestorof",   .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ANCESTOROF,   .flags = FOXY_KW_FLAG_NONE},
    {.text = "descendantof", .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_DESCENDANTOF, .flags = FOXY_KW_FLAG_NONE},
    {.text = "parentof",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PARENTOF,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "childof",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CHILDOF,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "typeof",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_TYPEOF,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "true",         .category = FOXY_TOKEN_CAT_LITERAL, .subtype = FOX_TOKEN_KW_TRUE,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "false",        .category = FOXY_TOKEN_CAT_LITERAL, .subtype = FOX_TOKEN_KW_FALSE,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "private",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PRIVATE,      .flags = FOXY_KW_FLAG_NONE},
    {.text = "protected",    .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PROTECTED,    .flags = FOXY_KW_FLAG_NONE},
    {.text = "public",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PUBLIC,       .flags = FOXY_KW_FLAG_NONE},

    /* Flagged Methods / Métodos Marcados */
    {.text = "__new",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NEW,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__cast",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CAST,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "__tostring",   .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_TOSTRING,   .flags = FOXY_KW_FLAG_NONE},
    {.text = "__add",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_ADD,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__sub",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_SUB,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__mul",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_MUL,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__div",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_DIV,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__pow",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_POW,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__mod",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_MOD,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__concat",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CONCAT,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "__unm",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_UNM,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__not",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NOT,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__eq",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_EQ,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "__neq",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NEQ,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__lt",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LT,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "__gt",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_GT,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "__le",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LE,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "__ge",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_GE,         .flags = FOXY_KW_FLAG_NONE},
    {.text = "__band",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BAND,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "__bor",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BOR,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__bnot",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BNOT,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "__bxor",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BXOR,       .flags = FOXY_KW_FLAG_NONE},
    {.text = "__lshift",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LSHIFT,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "__rshift",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_RSHIFT,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "__foreach",    .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_FOREACH,    .flags = FOXY_KW_FLAG_NONE},
    {.text = "__closed",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CLOSED,     .flags = FOXY_KW_FLAG_NONE},
    {.text = "__len",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LEN,        .flags = FOXY_KW_FLAG_NONE},
    {.text = "__unpack",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_UNPACK,     .flags = FOXY_KW_FLAG_NONE}
};

const size_t FOXY_KEYWORD_TABLE_SIZE = sizeof(FOXY_KEYWORD_TABLE) / sizeof(FoxyKeywordMap);

/* ========================================================================= */
/* AUXILIARES DE ENTRADA Y BUFFER DE LÍNEA                                   */
/* ========================================================================= */

// static bool f_lexer_load_next_line(FoxyLexer *lexer) {
//     if (!lexer || !lexer->file || lexer->is_eof) return false;
// 
//     if (foxy_fgets(lexer->line_buffer, sizeof(lexer->line_buffer), lexer->file) == NULL) {
//         /* Archivo vacío o se alcanzó el EOF -> cerrar recursos */
//         f_lexer_close(lexer);
//         return false;
//     }
// 
//     lexer->line++;
//     lexer->column = 1;
//     lexer->cursor = lexer->line_buffer;
//     lexer->token_start = lexer->line_buffer;
//     return true;
// }

static char f_lexer_peek(FoxyLexer *lexer) {
    if (!lexer || !lexer->cursor) return '\0';
    return *lexer->cursor;
}

static char f_lexer_peek_next(FoxyLexer *lexer) {
    if (!lexer || !lexer->cursor || *lexer->cursor == '\0') return '\0';
    return *(lexer->cursor + 1);
}

static char f_lexer_advance(FoxyLexer *lexer) {
    char c = f_lexer_peek(lexer);
    if (c != '\0') {
        lexer->cursor++;
        lexer->column++;
    }
    return c;
}

static bool f_lexer_match(FoxyLexer *lexer, char expected) {
    if (f_lexer_peek(lexer) == expected) {
        f_lexer_advance(lexer);
        return true;
    }
    return false;
}

static FoxyToken f_lexer_make_token(FoxyLexer *lexer, uint8_t cat, FoxyTokenType type) {
    FoxyToken token;
    token.start = lexer->token_start;
    token.length = (uint32_t)(lexer->cursor - lexer->token_start);
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column - token.length;
    token.type_category = cat;
    token.type = f_foxmode_compose_type(cat, (uint16_t)type);
    token.flags = 0;
    return token;
}

static FoxyToken f_lexer_make_error_token(FoxyLexer *lexer, const char *msg) {
    FoxyToken token;
    token.start = msg;
    token.length = (uint32_t)strlen(msg);
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column;
    token.type_category = FOXY_TOKEN_CAT_ERROR;
    token.type = FOX_TOKEN_ERROR;
    token.flags = 0;
    return token;
}

static void f_lexer_skip_whitespace_and_comments(FoxyLexer *lexer) {
    for (;;) {
        char c = f_lexer_peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                f_lexer_advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 1;
                f_lexer_advance(lexer);
                break;
            case '@': {
                size_t count = 0;
                while (f_lexer_peek(lexer) == '@') {
                    count++;
                    f_lexer_advance(lexer);
                }

                if (count == 1) {
                    /* Comentario de línea única (@ ... \n) */
                    while (f_lexer_peek(lexer) != '\n' && f_lexer_peek(lexer) != '\0') {
                        f_lexer_advance(lexer);
                    }
                } else {
                    /* Comentario multilínea (N@ ... N@) */
                    int closed = 0;
                    while (f_lexer_peek(lexer) != '\0') {
                        if (f_lexer_peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 1;
                            f_lexer_advance(lexer);
                            continue;
                        }

                        if (f_lexer_peek(lexer) == '@') {
                            size_t close_count = 0;
                            while (f_lexer_peek(lexer) == '@') {
                                close_count++;
                                f_lexer_advance(lexer);
                            }

                            if (close_count == count) {
                                closed = 1;
                                break; /* Cerrado correctamente con el nivel exacto */
                            }
                        } else {
                            f_lexer_advance(lexer);
                        }
                    }

                    if (!closed) {
                        f_lexer_error(lexer, "Unclosed multi-line comment starting at level %zu.", count);
                    }
                }
                break;
            }
            default:
                return; /* Al ser void, simplemente retornamos sin valor */
        }
    }
}
/*
static char f_lexer_decode_escape(const char **cursor_ptr) {
    const char *p = *cursor_ptr;
    if (*p != '\\') {
        char c = *p;
        (*cursor_ptr)++;
        return c;
    }
    p++; // Saltar '\'
    char c = *p++;
    switch (c) {
        case 'n':  *cursor_ptr = p; return '\n'; // Newline
        case 'r':  *cursor_ptr = p; return '\r'; // Carriage Return
        case 't':  *cursor_ptr = p; return '\t'; // Horizontal Tab
        case 'b':  *cursor_ptr = p; return '\b'; // Backspace
        case 'a':  *cursor_ptr = p; return '\a'; // Alert
        case 'f':  *cursor_ptr = p; return '\f'; // Form Feed
        case 'v':  *cursor_ptr = p; return '\v'; // Vertical Tab
        case '\\': *cursor_ptr = p; return '\\'; // Backslash
        case '\'': *cursor_ptr = p; return '\''; // Single Quote
        case '\"': *cursor_ptr = p; return '\"'; // Double Quote
        case '?':  *cursor_ptr = p; return '\?'; // Question Mark
        case '0':  *cursor_ptr = p; return '\0'; // Null Character
        case 'x': {
            // Hex value: \xhh
            int val = 0;
            for (int i = 0; i < 2 && isxdigit((unsigned char)*p); i++) {
                int digit = *p;
                val = val * 16 + (isdigit(digit) ? digit - '0' : (tolower(digit) - 'a' + 10));
                p++;
            }
            *cursor_ptr = p;
            return (char)val;
        }
        default: {
            // Octal value: \ooo (hasta 3 dígitos)
            if (c >= '0' && c <= '7') {
                int val = c - '0';
                for (int i = 0; i < 2 && *p >= '0' && *p <= '7'; i++) {
                    val = val * 8 + (*p - '0');
                    p++;
                }
                *cursor_ptr = p;
                return (char)val;
            }
            *cursor_ptr = p;
            return c;
        }
    }
}
*/
static const FoxyRegisteredType *f_lexer_find_known_type(FoxyLexer *lexer, const char *start, size_t length) {
    if (!lexer) return NULL;
    for (size_t i = 0; i < lexer->known_types.count; ++i) {
        if (strlen(lexer->known_types.items[i].name) == length &&
            memcmp(lexer->known_types.items[i].name, start, length) == 0) {
            return &lexer->known_types.items[i];
        }
    }
    return NULL;
}

/* ========================================================================= */
/* SCANNING DE TOKENS COMPLEJOS                                              */
/* ========================================================================= */

static FoxyToken f_lexer_scan_identifier(FoxyLexer *lexer) {
    const char *start_pos = lexer->token_start;
    while (isalnum((unsigned char)f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }

    uint32_t len = (uint32_t)(lexer->cursor - start_pos);
    
    /* 1. Buscar en la tabla estática de palabras clave */
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; ++i) {
        if (strlen(FOXY_KEYWORD_TABLE[i].text) == len &&
            memcmp(start_pos, FOXY_KEYWORD_TABLE[i].text, len) == 0) {
            
            FoxyToken token = f_lexer_make_token(lexer, (uint8_t)FOXY_KEYWORD_TABLE[i].category, (FoxyTokenType)FOXY_KEYWORD_TABLE[i].subtype);
            token.flags = FOXY_KEYWORD_TABLE[i].flags;
            return token;
        }
    }

    /* 2. Verificar si ya está registrado en known_types */
    const FoxyRegisteredType *known_type = f_lexer_find_known_type(lexer, start_pos, len);
    if (known_type) {
        // Si el nodo anterior es asignación o separador, es un uso del tipo personalizado
        if (lexer->node_current && 
            (lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ASSIGN) ||
             lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COMMA) ||
             lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LPAREN))) {
            FoxyToken token = f_lexer_make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_CUSTOM_DATATYPE_IDENTIFIER);
            token.flags = known_type->flags;
            return token;
        }
        FoxyToken token = f_lexer_make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CUSTOM_DATATYPE);
        token.flags = known_type->flags;
        return token;
    }

    /* 3. Análisis Profundo mediante Lookahead (Detección de Declaración de Variable) */
    /* Patrones: [global] [static] [const] CustomType variable = ... */
    FoxyLexer state_peek = *lexer;
    FoxyToken next_tok = f_lexer_next_token(&state_peek);
    
    bool is_declaration = false;
    // Si el siguiente token es un identificador (nombre de la variable) y luego hay '=' o '('
    if (next_tok.type == f_lexer_compose_type(FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_IDENTIFIER) ||
        next_tok.type == f_lexer_compose_type(FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_IDENTIFIER)) {
        
        FoxyToken next_next_tok = f_lexer_next_token(&state_peek);
        if (next_next_tok.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ASSIGN) ||
            next_next_tok.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LPAREN)) {
            is_declaration = true;
        }
    }

    if (is_declaration) {
        // Pre-registro automático del tipo personalizado detectado
        f_lexer_register_type(lexer, start_pos, len, FOXY_KW_FLAG_CUSTOM_TYPE);
        FoxyToken token = f_lexer_make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CUSTOM_DATATYPE);
        token.flags = FOXY_KW_FLAG_CUSTOM_TYPE;
        return token;
    }

    /* 4. Si aparece después de '=' o '(' en el lado derecho (instanciación/constructor) */
    if (lexer->node_current && 
        (lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ASSIGN) ||
         lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COMMA) ||
         lexer->node_current->token.type == f_lexer_compose_type(FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LPAREN))) {
        
        f_lexer_register_type(lexer, start_pos, len, FOXY_KW_FLAG_CUSTOM_TYPE);
        FoxyToken token = f_lexer_make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_CUSTOM_DATATYPE_IDENTIFIER);
        token.flags = FOXY_KW_FLAG_CUSTOM_TYPE;
        return token;
    }

    /* 5. Identificador estándar por defecto */
    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_IDENTIFIER);
}

/* Helper para saltar guiones bajos consecutivos */
static inline void f_lexer_skip_underscores(FoxyLexer *lexer) {
    while (f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }
}

static FoxyToken f_lexer_scan_number(FoxyLexer *lexer) {
    bool is_float = false;
    int base = 10;

    /* 1. Detección de base (16, 2, 8, 10) */
    if (lexer->token_start[0] == '0') {
        switch (f_lexer_peek(lexer) | 0x20) {
            case 'x': base = 16; f_lexer_advance(lexer); break;
            case 'b': base = 2;  f_lexer_advance(lexer); break;
            case 'o': base = 8;  f_lexer_advance(lexer); break;
            default: break;
        }
    }

    /* 2. Lectura del cuerpo numérico */
    for (;;) {
        f_lexer_skip_underscores(lexer);
        char c = f_lexer_peek(lexer);

        switch (base) {
            case 16: if (!isxdigit((unsigned char)c)) goto check_float; break;
            case 2:  if (c != '0' && c != '1') goto check_float; break;
            case 8:  if (c < '0' || c > '7') goto check_float; break;
            default: if (!isdigit((unsigned char)c)) goto check_float; break;
        }
        f_lexer_advance(lexer);
    }

check_float:
    /* 3. Detección de punto flotante */
    if (base == 10 && f_lexer_peek(lexer) == '.') {
        char next = f_lexer_peek_next(lexer);
        if (isdigit((unsigned char)next) || next == '_') {
            is_float = true;
            f_lexer_advance(lexer); // Consumir '.'
            for (;;) {
                f_lexer_skip_underscores(lexer);
                if (!isdigit((unsigned char)f_lexer_peek(lexer))) break;
                f_lexer_advance(lexer);
            }
        }
    }

    /* 4. Captura de la región del sufijo */
    const char *suffix_start = lexer->cursor;
    while (isalpha((unsigned char)f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }
    size_t suffix_len = (size_t)(lexer->cursor - suffix_start);

    /* 5. Resolución mediante la Tabla de Reglas (Hash/Lookup Table Estática) */
    FoxyTokenType token_type = is_float ? FOX_TOKEN_DOUBLE_LITERAL : FOX_TOKEN_INT_LITERAL;

    if (suffix_len > 0) {
        bool matched = false;
        for (size_t i = 0; i < FOXY_SUFFIX_TABLE_SIZE; ++i) {
            if (FOXY_SUFFIX_TABLE[i].length == suffix_len &&
                memcmp(suffix_start, FOXY_SUFFIX_TABLE[i].suffix, suffix_len) == 0) {
                token_type = FOXY_SUFFIX_TABLE[i].token_type;
                matched = true;
                break;
            }
        }

        if (!matched) {
            /* Sufijo inválido o malformado: consumimos el resto si es necesario y lanzamos error */
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_ERROR, (FoxyTokenType)FOX_EXCEPTION_MALFORMED_NUMBER);
        }
    }

    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_LITERAL, token_type);
}

static FoxyToken f_lexer_scan_string(FoxyLexer *lexer) {
    while (f_lexer_peek(lexer) != '"' && f_lexer_peek(lexer) != '\0') {
        if (f_lexer_peek(lexer) == '\\' && f_lexer_peek_next(lexer) != '\0') {
            f_lexer_advance(lexer);
        }
        f_lexer_advance(lexer);
    }

    if (f_lexer_peek(lexer) == '\0') {
        return f_lexer_make_error_token(lexer, "Unterminated string literal.");
    }

    f_lexer_advance(lexer); /* Consumir las comillas de cierre '"' */
    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_STRING_LITERAL);
}

static FoxyToken f_lexer_scan_char(FoxyLexer *lexer) {
    if (f_lexer_peek(lexer) == '\\') {
        f_lexer_advance(lexer);
        if (f_lexer_peek(lexer) == 'x') {
            f_lexer_advance(lexer);
            while (isxdigit((unsigned char)f_lexer_peek(lexer))) f_lexer_advance(lexer);
        } else if (f_lexer_peek(lexer) >= '0' && f_lexer_peek(lexer) <= '7') {
            int count = 0;
            while (count < 3 && f_lexer_peek(lexer) >= '0' && f_lexer_peek(lexer) <= '7') {
                f_lexer_advance(lexer);
                count++;
            }
        } else if (f_lexer_peek(lexer) != '\0') {
            f_lexer_advance(lexer);
        }
    } else {
        f_lexer_advance(lexer);
    }

    if (f_lexer_peek(lexer) != '\'') {
        return f_lexer_make_error_token(lexer, "Unterminated character literal.");
    }

    f_lexer_advance(lexer); /* Consumir '\'' */
    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_CHAR_LITERAL);
}

/* ========================================================================= */
/* IMPLEMENTACIÓN DE LA API PÚBLICA                                           */
/* ========================================================================= */

FOXY_EXPORT void f_lexer_init_lexer(FoxyLexer *lexer, FoxyExceptionController *controller) {
    if (!lexer) return;
    memset(lexer, 0, sizeof(FoxyLexer));
    lexer->exception_controller = controller;

    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; ++i) {
        if (FOXY_KEYWORD_TABLE[i].category == FOXY_TOKEN_CAT_TYPE) {
            f_lexer_register_type(
                lexer, 
                FOXY_KEYWORD_TABLE[i].text, 
                strlen(FOXY_KEYWORD_TABLE[i].text),
                FOXY_KEYWORD_TABLE[i].flags
            );
        }
    }
}

FOXY_EXPORT bool f_lexer_init_string(FoxyLexer *lexer, const char *source, const char *filename, FoxyExceptionController *controller) {
    if (!lexer || !source) return false;
    
    f_lexer_init_lexer(lexer, controller);

    lexer->source = source;
    lexer->cursor = source;
    lexer->token_start = source;
    lexer->filename = filename ? filename : "<string>";
    lexer->line = 1;
    lexer->column = 1;
    lexer->owns_source = 0;
    lexer->is_eof = (*source == '\0');

    return true;
}

FOXY_EXPORT bool f_lexer_init_file(FoxyLexer *lexer, FILE *file, const char *filename, FoxyExceptionController *controller) {
    if (!lexer || !file) return false;
    
    f_lexer_init_lexer(lexer, controller);

    if (foxy_fseek(file, 0, SEEK_END) != 0) return false;
    foxy_off_t file_size = foxy_ftell(file);
    if (file_size < 0) return false;
    foxy_fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) return false;

    size_t bytes_read = foxy_fread(buffer, 1, (size_t)file_size, file);
    buffer[bytes_read] = '\0';

    lexer->file = file;
    lexer->source = buffer;
    lexer->cursor = buffer;
    lexer->token_start = buffer;
    lexer->filename = filename ? filename : "<unknown>";
    lexer->line = 1;
    lexer->column = 1;
    lexer->owns_source = 1;
    lexer->is_eof = (bytes_read == 0);

    return true;
}

FOXY_EXPORT void f_lexer_close(FoxyLexer *lexer) {
    if (!lexer) return;

    FoxyLexerNode *curr = lexer->node_current;
    while (curr) {
        FoxyLexerNode *prev = curr->previous;
        free(curr);
        curr = prev;
    }
    lexer->node_current = NULL;
    lexer->node_previous = NULL;

    if (lexer->known_types.items) {
        for (size_t i = 0; i < lexer->known_types.count; ++i) {
            free(lexer->known_types.items[i].name);
        }
        free(lexer->known_types.items);
        lexer->known_types.items = NULL;
        lexer->known_types.count = 0;
        lexer->known_types.capacity = 0;
    }

    if (lexer->owns_source && lexer->source) {
        free((void *)lexer->source);
        lexer->source = NULL;
    }

    if (lexer->file) {
        foxy_fclose(lexer->file);
        lexer->file = NULL;
    }

    lexer->cursor = NULL;
    lexer->token_start = NULL;
    lexer->is_eof = true;
}

static inline FoxyToken f_lexer_create_token(FoxyLexer *lexer, char c) {
    switch (c) {
        case '\'': return f_lexer_scan_char(lexer);
        case '"': return f_lexer_scan_string(lexer);
        case '(': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LPAREN);
        case ')': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RPAREN);
        case '{': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LBRACE);
        case '}': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RBRACE);
        case '[': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LBRACKET);
        case ']': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RBRACKET);
        case ';': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SEMICOLON);
        case ',': return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COMMA);
        case '.':
            if (f_lexer_match(lexer, '.')) {
                if (f_lexer_match(lexer, '.')) {
                    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DOTDOTDOT);
                }
                return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DOTDOT);
            }
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DOT);
        case '+':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PLUS_ASSIGN);
            if (f_lexer_match(lexer, '+')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_INC);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PLUS);
        case '-':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_MINUS_ASSIGN);
            if (f_lexer_match(lexer, '-')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DEC);
            if (f_lexer_match(lexer, '>')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PTR_ARROW);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_MINUS);
        case '*':
            if (f_lexer_match(lexer, '*')) {
                if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_POWER_ASSIGN);
                return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_POWER);
            }
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_STAR_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_STAR);
        case '/':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SLASH_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SLASH);
        case '=':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_EQ);
            if (f_lexer_match(lexer, '>')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_FAT_ARROW);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ASSIGN);
        case '!':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_NEQ);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_BANG);
        case '&':
            if (f_lexer_match(lexer, '&')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_AND);
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_AND_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_AMPERSAND);
        case '|':
            if (f_lexer_match(lexer, '|')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_OR);
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_OR_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PIPE);
        case '^':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_XOR_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_CARET);
        case '~':
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_TILDE);
        case '<':
            if (f_lexer_match(lexer, '<')) {
                if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LSHIFT_ASSIGN);
                return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LSHIFT);
            }
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LE);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LT);
        case '>':
            if (f_lexer_match(lexer, '>')) {
                if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RSHIFT_ASSIGN);
                return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RSHIFT);
            }
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_GE);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_GT);
        case '%':
            if (f_lexer_match(lexer, '=')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PERCENT_ASSIGN);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PERCENT);
        case ':':
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COLON);
        case '?': 
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_QUESTION);
        case '#':
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_HASH);
        default:
            break;
    }

    return f_lexer_make_error_token(lexer, "Unexpected character.");
}

FOXY_EXPORT FoxyToken f_lexer_next_token(FoxyLexer *lexer) {
    if (!lexer) return (FoxyToken){0};

    if (*lexer->cursor == '\0') {
        lexer->token_start = lexer->cursor;
        FoxyToken eof_tok = f_lexer_make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_EOF);
        return eof_tok;
    }

    f_lexer_skip_whitespace_and_comments(lexer);
    lexer->token_start = lexer->cursor;

    char c = f_lexer_peek(lexer);
    if (c == '\0') {
        lexer->is_eof = 1;
        return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_EOF);
    }

    f_lexer_advance(lexer);

    FoxyToken token;
    if (isalpha(c) || c == '_') {
        lexer->cursor = lexer->token_start; // Retroceder temporalmente para el escaneo completo
        f_lexer_advance(lexer);
        token = f_lexer_scan_identifier(lexer);
    } else if (isdigit(c)) {
        lexer->cursor = lexer->token_start;
        token = f_lexer_scan_number(lexer);
    } else {
        token = f_lexer_create_token(lexer, c);
    }

    FoxyLexerNode *new_node = (FoxyLexerNode *)malloc(sizeof(FoxyLexerNode));
    if (new_node) {
        new_node->token = token;
        new_node->previous = lexer->node_current;
        new_node->current = new_node;
        if (lexer->node_current) {
            lexer->node_current->current = new_node;
        }
        lexer->node_previous = lexer->node_current;
        lexer->node_current = new_node;
    }

    return token;
}

FOXY_EXPORT FoxyToken f_lexer_peek_token(FoxyLexer *lexer) {
    if (!lexer) return (FoxyToken){0};

    FoxyLexer state_copy = *lexer;
    FoxyToken token = f_lexer_next_token(&state_copy);
    return token;
}

FOXY_EXPORT const char *f_lexer_token_type_to_string(FoxyTokenType type) {
    uint32_t subtype = (uint32_t)type & FOXMODE_MASK_12BIT;
    if (subtype >= sizeof(FOXY_TOKEN_NAMES) / sizeof(FOXY_TOKEN_NAMES[0])) {
        return "FOX_TOKEN_UNKNOWN";
    }
    return FOXY_TOKEN_NAMES[subtype];
}

FOXY_EXPORT void f_lexer_print_token(const FoxyToken *token) {
    if (!token) return;
    printf("Token { type: %-24s (Cat: %u), lexeme: '%.*s', line: %u, col: %u, flags: 0x%X }\n",
           f_lexer_token_type_to_string(token->type),
           token->type_category,
           token->length, token->start,
           token->pos.line, token->pos.column,
           token->flags);
}

FOXY_EXPORT void f_lexer_error(FoxyLexer *lexer, const char *format, ...) {
    if (!lexer) return;
    
    if (lexer->exception_controller) {
        va_list args;
        va_start(args, format);
        // Soporte para lanzar excepción léxica general a través del controlador.
        // Aquí se puede personalizar el código según el error o usar FOX_EXCEPTION_SYNTAX_ERROR/INVALID_CHAR. . .
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        lexer->exception_controller->error_count++;
        lexer->exception_controller->had_error = true;
    } else {
        fprintf(stderr, "[Foxy Lexer Error] %s:%u:%u: ", 
                lexer->filename ? lexer->filename : "<unknown>", 
                lexer->line, 
                lexer->column);
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

FOXY_EXPORT void f_lexer_register_type(FoxyLexer *lexer, const char *name, size_t length, uint32_t flags) {
    if (!lexer || !name || length == 0) return;

    /* Evitar duplicados y actualizar flags si ya existe */
    for (size_t i = 0; i < lexer->known_types.count; ++i) {
        if (strlen(lexer->known_types.items[i].name) == length &&
            memcmp(lexer->known_types.items[i].name, name, length) == 0) {
            lexer->known_types.items[i].flags = flags;
            return;
        }
    }

    if (lexer->known_types.count + 1 > lexer->known_types.capacity) {
        size_t new_cap = lexer->known_types.capacity == 0 ? 8 : lexer->known_types.capacity * 2;
        FoxyRegisteredType *new_items = realloc(lexer->known_types.items, sizeof(FoxyRegisteredType) * new_cap);
        if (!new_items) return;
        lexer->known_types.items = new_items;
        lexer->known_types.capacity = new_cap;
    }

    char *type_name = malloc(length + 1);
    if (!type_name) return;
    memcpy(type_name, name, length);
    type_name[length] = '\0';

    lexer->known_types.items[lexer->known_types.count].name = type_name;
    lexer->known_types.items[lexer->known_types.count].flags = flags;
    lexer->known_types.count++;
}