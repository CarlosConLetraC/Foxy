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
    "FOX_TOKEN_EOF", "FOX_TOKEN_ERROR", "FOX_TOKEN_IDENTIFIER", "FOX_TOKEN_DATATYPE_IDENTIFIER", "FOX_TOKEN_LABEL",
    "FOX_TOKEN_INT_LITERAL", "FOX_TOKEN_UINT_LITERAL", "FOX_TOKEN_LONG_LITERAL",
    "FOX_TOKEN_ULONG_LITERAL", "FOX_TOKEN_LLONG_LITERAL", "FOX_TOKEN_ULLONG_LITERAL",
    "FOX_TOKEN_FLOAT_LITERAL", "FOX_TOKEN_DOUBLE_LITERAL", "FOX_TOKEN_LDOUBLE_LITERAL",
    "FOX_TOKEN_NUMBER_LITERAL", "FOX_TOKEN_CHAR_LITERAL", "FOX_TOKEN_STRING_LITERAL",
    "FOX_TOKEN_KW_GLOBAL", "FOX_TOKEN_KW_STATIC", "FOX_TOKEN_KW_CONST", "FOX_TOKEN_KW_NULL",
    "FOX_TOKEN_KW_BOOL", "FOX_TOKEN_KW_CHAR", "FOX_TOKEN_KW_UCHAR", "FOX_TOKEN_KW_SHORT",
    "FOX_TOKEN_KW_USHORT", "FOX_TOKEN_KW_INT", "FOX_TOKEN_KW_UINT", "FOX_TOKEN_KW_LONG",
    "FOX_TOKEN_KW_ULONG", "FOX_TOKEN_KW_LLONG", "FOX_TOKEN_KW_ULLONG", "FOX_TOKEN_KW_FLOAT",
    "FOX_TOKEN_KW_DOUBLE", "FOX_TOKEN_KW_LDOUBLE", "FOX_TOKEN_KW_NUMBER", "FOX_TOKEN_KW_DICT",
    "FOX_TOKEN_KW_IF", "FOX_TOKEN_KW_ELSE", "FOX_TOKEN_KW_ELSEIF",
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
    "FOX_TOKEN_FAT_ARROW", "FOX_TOKEN_PTR_ARROW",
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
    {.text = "__unpack",     .category = FOXY_TOKEN_CAT_METHOD,  .subtype = FOX_TOKEN_METHOD_UNPACK,     .flags = 0}
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

static bool f_lexer_is_known_type(FoxyLexer *lexer, const char *start, size_t length) {
    if (!lexer) return false;
    for (size_t i = 0; i < lexer->known_types.count; ++i) {
        if (strlen(lexer->known_types.items[i]) == length &&
            memcmp(lexer->known_types.items[i], start, length) == 0) {
            return true;
        }
    }
    return false;
}

/* ========================================================================= */
/* SCANNING DE TOKENS COMPLEJOS                                              */
/* ========================================================================= */

static FoxyToken f_lexer_scan_identifier(FoxyLexer *lexer) {
    while (isalnum((unsigned char)f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
        f_lexer_advance(lexer);
    }

    uint32_t len = (uint32_t)(lexer->cursor - lexer->token_start);
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; ++i) {
        if (strlen(FOXY_KEYWORD_TABLE[i].text) == len &&
            memcmp(lexer->token_start, FOXY_KEYWORD_TABLE[i].text, len) == 0) {
            return f_lexer_make_token(lexer, (uint8_t)FOXY_KEYWORD_TABLE[i].category, (FoxyTokenType)FOXY_KEYWORD_TABLE[i].subtype);
        }
    }

    /* Verificar si el identificador corresponde a un tipo de dato personalizado previamente registrado */
    if (f_lexer_is_known_type(lexer, lexer->token_start, len)) {
        return f_lexer_make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_DATATYPE_IDENTIFIER);
    }

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

FOXY_EXPORT void f_lexer_init_lexer(FoxyLexer *lexer) {
    if (!lexer) return;
    memset(lexer, 0, sizeof(FoxyLexer));

    /* Registrar automáticamente todos los tipos desde la tabla maestra de palabras clave */
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; ++i) {
        if (FOXY_KEYWORD_TABLE[i].category == FOXY_TOKEN_CAT_TYPE) {
            f_lexer_register_type(
                lexer, 
                FOXY_KEYWORD_TABLE[i].text, 
                strlen(FOXY_KEYWORD_TABLE[i].text)
            );
        }
    }
}

FOXY_EXPORT bool f_lexer_init_string(FoxyLexer *lexer, const char *source, const char *filename) {
    if (!lexer || !source) return false;
    
    f_lexer_init_lexer(lexer); // Limpia y registra los tipos automáticamente

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

FOXY_EXPORT bool f_lexer_init_file(FoxyLexer *lexer, FILE *file, const char *filename) {
    if (!lexer || !file) return false;
    
    f_lexer_init_lexer(lexer); // Limpia y registra los tipos automáticamente

    /* Obtener tamaño del archivo usando las macros de f_settings.h */
    if (foxy_fseek(file, 0, SEEK_END) != 0) return false;
    foxy_off_t file_size = foxy_ftell(file);
    if (file_size < 0) return false;
    foxy_fseek(file, 0, SEEK_SET);

    /* Reservar buffer contiguo en heap (+1 para '\0') */
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

    if (lexer->known_types.items) {
        for (size_t i = 0; i < lexer->known_types.count; ++i) {
            free((void *)lexer->known_types.items[i]);
        }
        free((void *)lexer->known_types.items);
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

FOXY_EXPORT FoxyToken f_lexer_next_token(FoxyLexer *lexer) {
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

FOXY_EXPORT FoxyToken f_lexer_peek_token(FoxyLexer *lexer) {
    if (!lexer) return (FoxyToken){0};

    FoxyLexer state_copy = *lexer;
    FoxyToken token = f_lexer_next_token(&state_copy);
    return token;
}

FOXY_EXPORT const char *f_lexer_token_type_to_string(FoxyTokenType type) {
    return FOXY_TOKEN_NAMES[type];
}

FOXY_EXPORT void f_lexer_print_token(const FoxyToken *token) {
    if (!token) return;
    printf("Token { type: %-24s (Cat: %u), lexeme: '%.*s', line: %u, col: %u }\n",
           f_lexer_token_type_to_string(token->type),
           token->type_category,
           token->length, token->start,
           token->pos.line, token->pos.column);
}

FOXY_EXPORT void f_lexer_error(FoxyLexer *lexer, const char *format, ...) {
    if (!lexer) return;
    
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

FOXY_EXPORT void f_lexer_register_type(FoxyLexer *lexer, const char *name, size_t length) {
    if (!lexer || !name || length == 0) return;

    /* Evitar duplicados */
    for (size_t i = 0; i < lexer->known_types.count; ++i) {
        if (strlen(lexer->known_types.items[i]) == length &&
            memcmp(lexer->known_types.items[i], name, length) == 0) {
            return;
        }
    }

    if (lexer->known_types.count + 1 > lexer->known_types.capacity) {
        size_t new_cap = lexer->known_types.capacity == 0 ? 8 : lexer->known_types.capacity * 2;
        const char **new_items = realloc((void *)lexer->known_types.items, sizeof(const char *) * new_cap);
        if (!new_items) return;
        lexer->known_types.items = new_items;
        lexer->known_types.capacity = new_cap;
    }

    char *type_name = malloc(length + 1);
    if (!type_name) return;
    memcpy(type_name, name, length);
    type_name[length] = '\0';
    lexer->known_types.items[lexer->known_types.count++] = type_name;
}