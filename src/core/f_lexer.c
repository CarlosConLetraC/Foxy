#include "f_lexer.h"
#include "f_exceptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    "FOX_TOKEN_EOF", "FOX_TOKEN_ERROR", "FOX_TOKEN_IDENTIFIER", "FOX_TOKEN_LABEL",
    "FOX_TOKEN_INT_LITERAL", "FOX_TOKEN_UINT_LITERAL", "FOX_TOKEN_LONG_LITERAL",
    "FOX_TOKEN_ULONG_LITERAL", "FOX_TOKEN_LLONG_LITERAL", "FOX_TOKEN_ULLONG_LITERAL",
    "FOX_TOKEN_FLOAT_LITERAL", "FOX_TOKEN_DOUBLE_LITERAL", "FOX_TOKEN_LDOUBLE_LITERAL",
    "FOX_TOKEN_NUMBER_LITERAL", "FOX_TOKEN_CHAR_LITERAL", "FOX_TOKEN_STRING_LITERAL",
    "FOX_TOKEN_KW_GLOBAL", "FOX_TOKEN_KW_STATIC", "FOX_TOKEN_KW_CONST", "FOX_TOKEN_KW_NULL",
    "FOX_TOKEN_KW_BOOL", "FOX_TOKEN_KW_CHAR", "FOX_TOKEN_KW_UCHAR", "FOX_TOKEN_KW_SHORT",
    "FOX_TOKEN_KW_USHORT", "FOX_TOKEN_KW_INT", "FOX_TOKEN_KW_UINT", "FOX_TOKEN_KW_LONG",
    "FOX_TOKEN_KW_ULONG", "FOX_TOKEN_KW_LLONG", "FOX_TOKEN_KW_ULLONG", "FOX_TOKEN_KW_FLOAT",
    "FOX_TOKEN_KW_DOUBLE", "FOX_TOKEN_KW_LDOUBLE", "FOX_TOKEN_KW_NUMBER", "FOX_TOKEN_KW_DICT",
    "FOX_TOKEN_KW_OBJECT", "FOX_TOKEN_KW_IF", "FOX_TOKEN_KW_ELSE", "FOX_TOKEN_KW_ELSEIF",
    "FOX_TOKEN_KW_WHILE", "FOX_TOKEN_KW_FOR", "FOX_TOKEN_KW_FOREACH", "FOX_TOKEN_KW_SWITCH",
    "FOX_TOKEN_KW_CASE", "FOX_TOKEN_KW_DEFAULT", "FOX_TOKEN_KW_BREAK", "FOX_TOKEN_KW_CONTINUE",
    "FOX_TOKEN_KW_RETURN", "FOX_TOKEN_KW_GOTO", "FOX_TOKEN_KW_TRY", "FOX_TOKEN_KW_CATCH",
    "FOX_TOKEN_KW_EXCEPT", "FOX_TOKEN_KW_FINAL", "FOX_TOKEN_KW_CLASS", "FOX_TOKEN_KW_STRUCT",
    "FOX_TOKEN_KW_ENUM", "FOX_TOKEN_KW_FROM", "FOX_TOKEN_KW_FUNCTION", "FOX_TOKEN_KW_OVERRULE",
    "FOX_TOKEN_KW_INCLUDE", "FOX_TOKEN_KW_USE", "FOX_TOKEN_KW_EXPORT", "FOX_TOKEN_KW_SUPER",
    "FOX_TOKEN_KW_SELF", "FOX_TOKEN_KW_ANCESTOROF", "FOX_TOKEN_KW_DESCENDANTOF",
    "FOX_TOKEN_KW_PARENTOF", "FOX_TOKEN_KW_CHILDOF", "FOX_TOKEN_KW_TYPEOF", "FOX_TOKEN_KW_TRUE",
    "FOX_TOKEN_KW_FALSE", "FOX_TOKEN_KW_PRIVATE", "FOX_TOKEN_KW_PROTECTED", "FOX_TOKEN_KW_PUBLIC",
    "FOX_TOKEN_MOD_PRIVATE", "FOX_TOKEN_MOD_PROTECTED", "FOX_TOKEN_MOD_PUBLIC",
    "FOX_TOKEN_METHOD_NEW", "FOX_TOKEN_METHOD_CAST", "FOX_TOKEN_METHOD_TOSTRING",
    "FOX_TOKEN_METHOD_ADD", "FOX_TOKEN_METHOD_SUB", "FOX_TOKEN_METHOD_MUL",
    "FOX_TOKEN_METHOD_DIV", "FOX_TOKEN_METHOD_POW", "FOX_TOKEN_METHOD_MOD",
    "FOX_TOKEN_METHOD_CONCAT", "FOX_TOKEN_METHOD_UNM", "FOX_TOKEN_METHOD_NOT",
    "FOX_TOKEN_METHOD_EQ", "FOX_TOKEN_METHOD_NEQ", "FOX_TOKEN_METHOD_LT",
    "FOX_TOKEN_METHOD_GT", "FOX_TOKEN_METHOD_LE", "FOX_TOKEN_METHOD_GE",
    "FOX_TOKEN_METHOD_BAND", "FOX_TOKEN_METHOD_BOR", "FOX_TOKEN_METHOD_BNOT",
    "FOX_TOKEN_METHOD_BXOR", "FOX_TOKEN_METHOD_LSHIFT", "FOX_TOKEN_METHOD_RSHIFT",
    "FOX_TOKEN_METHOD_FOREACH", "FOX_TOKEN_METHOD_CLOSED", "FOX_TOKEN_METHOD_LEN", "FOX_TOKEN_METHOD_UNPACK",
    "FOX_TOKEN_PLUS", "FOX_TOKEN_MINUS", "FOX_TOKEN_STAR", "FOX_TOKEN_SLASH",
    "FOX_TOKEN_PERCENT", "FOX_TOKEN_POWER", "FOX_TOKEN_HASH", "FOX_TOKEN_ASSIGN",
    "FOX_TOKEN_PLUS_ASSIGN", "FOX_TOKEN_MINUS_ASSIGN", "FOX_TOKEN_STAR_ASSIGN",
    "FOX_TOKEN_SLASH_ASSIGN", "FOX_TOKEN_PERCENT_ASSIGN", "FOX_TOKEN_POWER_ASSIGN",
    "FOX_TOKEN_AND_ASSIGN", "FOX_TOKEN_OR_ASSIGN", "FOX_TOKEN_XOR_ASSIGN",
    "FOX_TOKEN_LSHIFT_ASSIGN", "FOX_TOKEN_RSHIFT_ASSIGN", "FOX_TOKEN_INC", "FOX_TOKEN_DEC",
    "FOX_TOKEN_EQ", "FOX_TOKEN_NEQ", "FOX_TOKEN_LT", "FOX_TOKEN_GT", "FOX_TOKEN_LE",
    "FOX_TOKEN_GE", "FOX_TOKEN_BANG", "FOX_TOKEN_AND", "FOX_TOKEN_OR", "FOX_TOKEN_AMPERSAND",
    "FOX_TOKEN_PIPE", "FOX_TOKEN_TILDE", "FOX_TOKEN_CARET", "FOX_TOKEN_LSHIFT", "FOX_TOKEN_RSHIFT",
    "FOX_TOKEN_ARROW", "FOX_TOKEN_FAT_ARROW", "FOX_TOKEN_PTR_ARROW", "FOX_TOKEN_ELLIPSIS",
    "FOX_TOKEN_LPAREN", "FOX_TOKEN_RPAREN", "FOX_TOKEN_LBRACE", "FOX_TOKEN_RBRACE",
    "FOX_TOKEN_LBRACKET", "FOX_TOKEN_RBRACKET", "FOX_TOKEN_SEMICOLON", "FOX_TOKEN_COLON",
    "FOX_TOKEN_COMMA", "FOX_TOKEN_DOT", "FOX_TOKEN_DOTDOT", "FOX_TOKEN_DOTDOTDOT", "FOX_TOKEN_QUESTION"
};
#endif

/* ========================================================================= */
/* TABLA DE PALABRAS CLAVE RESERVADAS                                        */
/* ========================================================================= */
const FoxyKeywordMap FOXY_KEYWORD_TABLE[] = {
    /* Modificadores / Especificadores */
    {.text = "global",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_GLOBAL,       .flags = 0},
    {.text = "static",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_STATIC,       .flags = 0},
    {.text = "const",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CONST,        .flags = 0},

    /* Palabras Reservadas / Primitivos */
    {.text = "null",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_NULL,         .flags = 0},
    {.text = "bool",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_BOOL,         .flags = 0},
    {.text = "char",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_CHAR,         .flags = 0},
    {.text = "uchar",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_UCHAR,        .flags = 0},
    {.text = "short",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_SHORT,        .flags = 0},
    {.text = "ushort",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_USHORT,       .flags = 0},
    {.text = "int",          .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_INT,          .flags = 0},
    {.text = "uint",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_UINT,         .flags = 0},
    {.text = "long",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LONG,         .flags = 0},
    {.text = "ulong",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_ULONG,        .flags = 0},
    {.text = "llong",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LLONG,        .flags = 0},
    {.text = "ullong",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_ULLONG,       .flags = 0},
    {.text = "float",        .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_FLOAT,        .flags = 0},
    {.text = "double",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_DOUBLE,       .flags = 0},
    {.text = "ldouble",      .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_LDOUBLE,      .flags = 0},
    {.text = "number",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_NUMBER,       .flags = 0},
    {.text = "dict",         .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_DICT,         .flags = 0},
    {.text = "object",       .category = FOXY_TOKEN_CAT_TYPE,    .subtype = FOX_TOKEN_KW_OBJECT,       .flags = 0},

    /* Control de Flujo */
    {.text = "if",           .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_IF,           .flags = 0},
    {.text = "else",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ELSE,         .flags = 0},
    {.text = "elseif",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ELSEIF,       .flags = 0},
    {.text = "while",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_WHILE,        .flags = 0},
    {.text = "for",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FOR,          .flags = 0},
    {.text = "foreach",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FOREACH,      .flags = 0},
    {.text = "switch",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SWITCH,       .flags = 0},
    {.text = "case",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CASE,         .flags = 0},
    {.text = "default",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_DEFAULT,      .flags = 0},
    {.text = "break",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_BREAK,        .flags = 0},
    {.text = "continue",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CONTINUE,     .flags = 0},
    {.text = "return",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_RETURN,       .flags = 0},
    {.text = "goto",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_GOTO,         .flags = 0},
    {.text = "try",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_TRY,          .flags = 0},
    {.text = "catch",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CATCH,        .flags = 0},
    {.text = "except",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_EXCEPT,       .flags = 0},
    {.text = "final",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FINAL,        .flags = 0},

    /* POO y Estructuras */
    {.text = "class",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CLASS,        .flags = 0},
    {.text = "struct",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_STRUCT,       .flags = 0},
    {.text = "enum",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ENUM,         .flags = 0},
    {.text = "from",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FROM,         .flags = 0},
    {.text = "function",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_FUNCTION,     .flags = 0},
    {.text = "overrule",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_OVERRULE,     .flags = 0},
    {.text = "include",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_INCLUDE,      .flags = 0},
    {.text = "use",          .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_USE,          .flags = 0},
    {.text = "export",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_EXPORT,       .flags = 0},
    {.text = "super",        .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SUPER,        .flags = 0},
    {.text = "self",         .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_SELF,         .flags = 0},
    {.text = "ancestorof",   .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_ANCESTOROF,   .flags = 0},
    {.text = "descendantof", .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_DESCENDANTOF, .flags = 0},
    {.text = "parentof",     .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PARENTOF,     .flags = 0},
    {.text = "childof",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_CHILDOF,      .flags = 0},
    {.text = "typeof",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_TYPEOF,       .flags = 0},
    {.text = "true",         .category = FOXY_TOKEN_CAT_LITERAL, .subtype = FOX_TOKEN_KW_TRUE,         .flags = 0},
    {.text = "false",        .category = FOXY_TOKEN_CAT_LITERAL, .subtype = FOX_TOKEN_KW_FALSE,        .flags = 0},
    {.text = "private",      .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PRIVATE,      .flags = 0},
    {.text = "protected",    .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PROTECTED,    .flags = 0},
    {.text = "public",       .category = FOXY_TOKEN_CAT_KEYWORD, .subtype = FOX_TOKEN_KW_PUBLIC,       .flags = 0},

    /* Flagged Methods / Métodos Marcados */
    {.text = "__new",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NEW,        .flags = 0},
    {.text = "__cast",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CAST,       .flags = 0},
    {.text = "__tostring",   .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_TOSTRING,   .flags = 0},
    {.text = "__add",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_ADD,        .flags = 0},
    {.text = "__sub",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_SUB,        .flags = 0},
    {.text = "__mul",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_MUL,        .flags = 0},
    {.text = "__div",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_DIV,        .flags = 0},
    {.text = "__pow",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_POW,        .flags = 0},
    {.text = "__mod",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_MOD,        .flags = 0},
    {.text = "__concat",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CONCAT,     .flags = 0},
    {.text = "__unm",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_UNM,        .flags = 0},
    {.text = "__not",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NOT,        .flags = 0},
    {.text = "__eq",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_EQ,         .flags = 0},
    {.text = "__neq",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_NEQ,        .flags = 0},
    {.text = "__lt",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LT,         .flags = 0},
    {.text = "__gt",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_GT,         .flags = 0},
    {.text = "__le",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LE,         .flags = 0},
    {.text = "__ge",         .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_GE,         .flags = 0},
    {.text = "__band",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BAND,       .flags = 0},
    {.text = "__bor",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BOR,        .flags = 0},
    {.text = "__bnot",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BNOT,       .flags = 0},
    {.text = "__bxor",       .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_BXOR,       .flags = 0},
    {.text = "__lshift",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LSHIFT,     .flags = 0},
    {.text = "__rshift",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_RSHIFT,     .flags = 0},
    {.text = "__foreach",    .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_FOREACH,    .flags = 0},
    {.text = "__closed",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_CLOSED,     .flags = 0},
    {.text = "__len",        .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_LEN,        .flags = 0},
    {.text = "__unpack",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_UNPACK}
};

const size_t FOXY_KEYWORD_TABLE_SIZE = sizeof(FOXY_KEYWORD_TABLE) / sizeof(FoxyKeywordMap);

/* ========================================================================= */
/* AUXILIARES DE ENTRADA Y BUFFER DE LÍNEA                                   */
/* ========================================================================= */

// static bool f_lexer_load_next_line(FoxyLexer *lexer) {
//     if (!lexer || !lexer->file || lexer->is_eof) return false;
// 
//     if (fgets(lexer->line_buffer, sizeof(lexer->line_buffer), lexer->file) == NULL) {
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
    token.type = type;
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
                lexer->cursor++; /* Avanza sin incrementar columna */
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
                    while (f_lexer_peek(lexer) != '\0') {
                        if (f_lexer_peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 1;
                            lexer->cursor++;
                            continue;
                        }

                        if (f_lexer_peek(lexer) == '@') {
                            size_t close_count = 0;
                            // const char *saved_cursor = lexer->cursor;
                            while (f_lexer_peek(lexer) == '@') {
                                close_count++;
                                f_lexer_advance(lexer);
                            }

                            if (close_count == count) {
                                break; /* Cerrado correctamente */
                            }
                        } else {
                            f_lexer_advance(lexer);
                        }
                    }
                }
                break;
            }
            default:
                return;
        }
    }
}

/* ========================================================================= */
/* SCANNING DE TOKENS COMPLEJOS                                              */
/* ========================================================================= */

static FoxyToken f_lexer_scan_identifier(FoxyLexer *lexer) {
    while (isalnum(f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }

    uint32_t len = (uint32_t)(lexer->cursor - lexer->token_start);
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; ++i) {
        if (strlen(FOXY_KEYWORD_TABLE[i].text) == len &&
            memcmp(lexer->token_start, FOXY_KEYWORD_TABLE[i].text, len) == 0) {
            return f_lexer_make_token(lexer, (uint8_t)FOXY_KEYWORD_TABLE[i].category, (FoxyTokenType)FOXY_KEYWORD_TABLE[i].subtype);
        }
    }

    return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_IDENTIFIER);
}

/* Matriz de resolución de tokens enteros: [has_u][l_count] */
static const FoxyTokenType INT_TOKEN_MAP[2][3] = {
    /* [has_u = 0] */ { FOX_TOKEN_INT_LITERAL,  FOX_TOKEN_LONG_LITERAL,  FOX_TOKEN_LLONG_LITERAL  },
    /* [has_u = 1] */ { FOX_TOKEN_UINT_LITERAL, FOX_TOKEN_ULONG_LITERAL, FOX_TOKEN_ULLONG_LITERAL }
};

/* Helper para saltar guiones bajos consecutivos */
static inline void f_lexer_skip_underscores(FoxyLexer *lexer) {
    while (f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }
}

static FoxyToken f_lexer_scan_number(FoxyLexer *lexer) {
    bool is_float = false;
    int base = 10;

    /* 1. Detección de base mediante switch */
    if (lexer->token_start[0] == '0') {
        switch (f_lexer_peek(lexer) | 0x20) {
            case 'x': base = 16; f_lexer_advance(lexer); break;
            case 'b': base = 2;  f_lexer_advance(lexer); break;
            case 'o': base = 8;  f_lexer_advance(lexer); break;
            default: break;
        }
    }

    /* 2. Lectura del cuerpo según la base (ignorando '_') */
    for (;;) {
        f_lexer_skip_underscores(lexer);
        char c = f_lexer_peek(lexer);

        switch (base) {
            case 16:
                if (!isxdigit((unsigned char)c)) goto check_float;
                break;
            case 2:
                if (c != '0' && c != '1') goto check_float;
                break;
            case 8:
                if (c < '0' || c > '7') goto check_float;
                break;
            default: /* Base 10 */
                if (!isdigit((unsigned char)c)) goto check_float;
                break;
        }
        f_lexer_advance(lexer);
    }

check_float:
    /* 3. Detección y consumo de punto flotante (Solo Base 10) */
    if (base == 10 && f_lexer_peek(lexer) == '.') {
        /* Miramos si tras el punto (o posibles '_') hay un dígito */
        char next = f_lexer_peek_next(lexer);
        if (isdigit((unsigned char)next) || next == '_') {
            is_float = true;
            f_lexer_advance(lexer); /* Consumir '.' */

            for (;;) {
                f_lexer_skip_underscores(lexer);
                if (!isdigit((unsigned char)f_lexer_peek(lexer))) break;
                f_lexer_advance(lexer);
            }
        }
    }

    if (base != 10) goto parse_suffixes;

parse_suffixes:;
    /* 4. Captura de sufijos (U, L, LL, F, D, LD) */
    bool has_u = false;
    int l_count = 0;
    bool has_f = false;
    bool has_d = false;

    for (;;) {
        char lower_c = f_lexer_peek(lexer) | 0x20;

        switch (lower_c) {
            case 'u':
                if (has_u || is_float || has_d) goto finalize_type;
                has_u = true;
                f_lexer_advance(lexer);
                continue;

            case 'l':
                if (l_count >= 2 || has_d || has_f) goto finalize_type;
                l_count++;
                f_lexer_advance(lexer);
                continue;

            case 'f':
                if (has_f || has_d || has_u || base != 10) goto finalize_type;
                has_f = is_float = true;
                f_lexer_advance(lexer);
                goto finalize_type;

            case 'd':
                /* Habilita 'd' como sufijo double o 'ld' / 'lld' para long double */
                if (has_f || has_d || has_u || base != 10) goto finalize_type;
                has_d = is_float = true;
                f_lexer_advance(lexer);
                goto finalize_type;

            default:
                goto finalize_type;
        }
    }

finalize_type:;
    /* Comprobar si el literal continúa con caracteres alfanuméricos no válidos (ej. 123ldx) */
    char next_c = f_lexer_peek(lexer);
    if (isalpha((unsigned char)next_c) || next_c == '_') {
        /* Se consume el identificador erróneo completo para evitar fragmentación de tokens */
        while (isalnum((unsigned char)f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
            f_lexer_advance(lexer);
        }
        /* Retorna token de error usando el código de excepción unificado */
        return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_ERROR, (FoxyTokenType)FOX_EXCEPTION_MALFORMED_NUMBER);
    }

    /* 5. Clasificación final de token */
    FoxyTokenType token_type;

    if (is_float) {
        token_type = has_f ? FOX_TOKEN_FLOAT_LITERAL :
                     (l_count > 0) ? FOX_TOKEN_LDOUBLE_LITERAL :
                                     FOX_TOKEN_DOUBLE_LITERAL;
    } else {
        token_type = INT_TOKEN_MAP[has_u ? 1 : 0][l_count > 2 ? 2 : l_count];
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

/* ========================================================================= */
/* IMPLEMENTACIÓN DE LA API PÚBLICA                                           */
/* ========================================================================= */

bool f_lexer_init_string(FoxyLexer *lexer, const char *source, const char *filename) {
    if (!lexer || !source) return false;
    memset(lexer, 0, sizeof(FoxyLexer));

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

bool f_lexer_init_file(FoxyLexer *lexer, FILE *file, const char *filename) {
    if (!lexer || !file) return false;
    memset(lexer, 0, sizeof(FoxyLexer));

    /* Obtener tamaño del archivo usando las macros de f_settings.h */
    if (foxy_fseek(file, 0, SEEK_END) != 0) return false;
    foxy_off_t file_size = foxy_ftell(file);
    if (file_size < 0) return false;
    foxy_fseek(file, 0, SEEK_SET);

    /* Reservar buffer contiguo en heap (+1 para '\0') */
    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) return false;

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
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

void f_lexer_close(FoxyLexer *lexer) {
    if (!lexer) return;

    if (lexer->owns_source && lexer->source) {
        free((void *)lexer->source);
        lexer->source = NULL;
    }

    if (lexer->file) {
        fclose(lexer->file);
        lexer->file = NULL;
    }

    lexer->cursor = NULL;
    lexer->token_start = NULL;
    lexer->is_eof = true;
}

FoxyToken f_lexer_next_token(FoxyLexer *lexer) {
    if (!lexer) return (FoxyToken){0};

    f_lexer_skip_whitespace_and_comments(lexer);

    lexer->token_start = lexer->cursor;

    char c = f_lexer_peek(lexer);
    if (c == '\0') {
        return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_EOF);
    }

    f_lexer_advance(lexer);

    if (isalpha(c) || c == '_') return f_lexer_scan_identifier(lexer);
    if (isdigit(c)) return f_lexer_scan_number(lexer);

    switch (c) {
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
            if (f_lexer_match(lexer, '>')) return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ARROW);
            return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_MINUS);
        case '*':
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

FoxyToken f_lexer_peek_token(FoxyLexer *lexer) {
    if (!lexer) return (FoxyToken){0};

    FoxyLexer state_copy = *lexer;
    FoxyToken token = f_lexer_next_token(&state_copy);
    return token;
}

const char *f_lexer_token_type_to_string(FoxyTokenType type) {
    return FOXY_TOKEN_NAMES[type];
}

void f_lexer_print_token(const FoxyToken *token) {
    if (!token) return;
    printf("Token { type: %-24s, lexeme: '%.*s', line: %u, col: %u }\n",
           f_lexer_token_type_to_string(token->type),
           token->length, token->start,
           token->pos.line, token->pos.column);
}