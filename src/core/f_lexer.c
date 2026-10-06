#include "f_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Tabla de palabras clave y métodos marcados con sus categorías */
const FoxyKeywordMap FOXY_KEYWORD_TABLE[] = {
    /* Modificadores / Especificadores */
    {"global",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_GLOBAL},
    {"static",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_STATIC},
    {"const",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CONST},

    /* Palabras Reservadas / Primitivos */
    {"null",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_NULL},
    {"bool",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_BOOL},
    {"char",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_CHAR},
    {"uchar",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_UCHAR},
    {"short",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_SHORT},
    {"ushort",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_USHORT},
    {"int",          FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_INT},
    {"uint",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_UINT},
    {"long",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LONG},
    {"ulong",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_ULONG},
    {"llong",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LLONG},
    {"ullong",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_ULLONG},
    {"float",        FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_FLOAT},
    {"double",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_DOUBLE},
    {"ldouble",      FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_LDOUBLE},
    {"number",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_NUMBER},
    {"dict",         FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_DICT},
    {"object",       FOXY_TOKEN_CAT_TYPE,    FOX_TOKEN_KW_OBJECT},

    /* Control de Flujo */
    {"if",           FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_IF},
    {"else",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSE},
    {"elseif",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSEIF},
    {"while",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_WHILE},
    {"for",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOR},
    {"foreach",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOREACH},
    {"switch",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SWITCH},
    {"case",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CASE},
    {"default",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DEFAULT},
    {"break",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_BREAK},
    {"continue",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CONTINUE},
    {"return",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_RETURN},
    {"goto",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_GOTO},
    {"try",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TRY},
    {"catch",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CATCH},
    {"except",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXCEPT},
    {"final",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FINAL},

    /* POO y Estructuras */
    {"class",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CLASS},
    {"struct",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_STRUCT},
    {"enum",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ENUM},
    {"from",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FROM},
    {"function",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FUNCTION},
    {"overrule",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_OVERRULE},
    {"include",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_INCLUDE},
    {"use",          FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_USE},
    {"export",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXPORT},
    {"super",        FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SUPER},
    {"self",         FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SELF},
    {"ancestorof",   FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ANCESTOROF},
    {"descendantof", FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DESCENDANTOF},
    {"parentof",     FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PARENTOF},
    {"childof",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CHILDOF},
    {"typeof",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TYPEOF},
    {"true",         FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_KW_TRUE},
    {"false",        FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_KW_FALSE},
    {"private",      FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PRIVATE},
    {"protected",    FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PROTECTED},
    {"public",       FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_PUBLIC},

    /* Flagged Methods / Métodos Marcados */
    {"__new",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_NEW},
    {"__cast",       FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_CAST},
    {"__tostring",   FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_TOSTRING},
    {"__add",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_ADD},
    {"__sub",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_SUB},
    {"__mul",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_MUL},
    {"__div",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_DIV},
    {"__pow",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_POW},
    {"__mod",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_MOD},
    {"__concat",     FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_CONCAT},
    {"__unm",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_UNM},
    {"__not",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_NOT},
    {"__eq",         FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_EQ},
    {"__neq",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_NEQ},
    {"__lt",         FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_LT},
    {"__gt",         FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_GT},
    {"__le",         FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_LE},
    {"__ge",         FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_GE},
    {"__band",       FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_BAND},
    {"__bor",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_BOR},
    {"__bnot",       FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_BNOT},
    {"__bxor",       FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_BXOR},
    {"__lshift",     FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_LSHIFT},
    {"__rshift",     FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_RSHIFT},
    {"__foreach",    FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_FOREACH},
    {"__closed",     FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_CLOSED},
    {"__len",        FOXY_TOKEN_CAT_METHOD,  FOX_TOKEN_METHOD_LEN}
};

const size_t FOXY_KEYWORD_TABLE_SIZE = sizeof(FOXY_KEYWORD_TABLE) / sizeof(FoxyKeywordMap);

static bool fetch_next_line(FoxyLexer *lexer) {
    if (lexer->is_eof || !lexer->file) return false;

    if (fgets(lexer->line_buffer, LEXER_LINE_BUFFER_SIZE, lexer->file) == NULL) {
        lexer->is_eof = true;
        lexer->line_buffer[0] = '\0';
        lexer->cursor = lexer->line_buffer;
        return false;
    }

    lexer->cursor = lexer->line_buffer;
    return true;
}

/* Auxiliares internos del Lexer */
static bool is_at_end(FoxyLexer *lexer) {
    if (*lexer->cursor == '\0') {
        if (!fetch_next_line(lexer)) return true;
    }
    return false;
}

static char advance(FoxyLexer *lexer) {
    if (is_at_end(lexer)) return '\0';

    char c = *lexer->cursor++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static char peek(FoxyLexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return *lexer->cursor;
}

static char peek_next(FoxyLexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    if (lexer->cursor[0] != '\0' && lexer->cursor[1] != '\0') return lexer->cursor[1];
    /* Si el siguiente carácter está al final de la línea actual, no se realiza lookahead multi-línea agresivo. */
    return '\0';
}

static bool match(FoxyLexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->cursor != expected) return false;
    advance(lexer);
    return true;
}

static FoxyToken make_token(FoxyLexer *lexer, FoxyTokenType type, uint16_t category) {
    FoxyToken token;
    token.type = type;
    token.start = lexer->token_start;
    token.length = (uint32_t)(lexer->cursor - lexer->token_start);
    token.type_category = category;
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column - token.length;
    return token;
}

static FoxyToken error_token(FoxyLexer *lexer, const char *message) {
    FoxyToken token;
    token.type = FOX_TOKEN_ERROR;
    token.start = message;
    token.length = (uint32_t)strlen(message);
    token.type_category = FOXY_TOKEN_CAT_ERROR;
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column;
    return token;
}

static void skip_whitespace_and_comments(FoxyLexer *lexer) {
    for (;;) {
        if (is_at_end(lexer)) return;

        char c = peek(lexer);
        switch (c) {
            case '\x20': // ascii 32 | char '\40' 
            case '\r':
            case '\t':
            case '\n':
                advance(lexer);
                break;
            case '@': {
                /* Conteo dinámico de símbolos '@' para comentarios simples y multilínea */
                size_t at_count = 0;
                const char *temp = lexer->cursor;
                while (*temp == '@') {
                    at_count++;
                    temp++;
                }

                if (at_count == 1) {
                    /* Comentario de una sola línea: @ ... \n */
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                } else {
                    /* Comentario multilínea: se requieren 'at_count' símbolos '@' para cerrar */
                    for (size_t i = 0; i < at_count; i++) advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '@') {
                            size_t close_count = 0;
                            const char *c_temp = lexer->cursor;
                            while (*c_temp == '@') {
                                close_count++;
                                c_temp++;
                            }
                            if (close_count == at_count) {
                                for (size_t i = 0; i < at_count; i++) advance(lexer);
                                break;
                            }
                        }
                        advance(lexer);
                    }
                }
                break;
            }
            default:
                return;
        }
    }
}

void f_lexer_init_file(FoxyLexer *lexer, FILE *file, const char *filename) {
    lexer->file = file;
    lexer->filename = filename ? filename : "script.foxy";
    lexer->line = 1;
    lexer->column = 1;
    lexer->is_eof = false;
    lexer->line_buffer[0] = '\0';
    lexer->cursor = lexer->line_buffer;
    lexer->token_start = lexer->line_buffer;

    fetch_next_line(lexer);
}

// void f_lexer_init(FoxyLexer *lexer, const char *source, const char *filename) {
//     lexer->source = source;
//     lexer->cursor = source;
//     lexer->token_start = source;
//     lexer->filename = filename ? filename : "script.foxy";
//     lexer->line = 1;
//     lexer->column = 1;
// }

static FoxyToken scan_identifier_or_keyword(FoxyLexer *lexer) {
    while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    size_t length = (size_t)(lexer->cursor - lexer->token_start);

    /* Buscar coincidencia en la tabla de palabras clave */
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; i++) {
        if (strlen(FOXY_KEYWORD_TABLE[i].text) == length &&
            memcmp(lexer->token_start, FOXY_KEYWORD_TABLE[i].text, length) == 0) {
            return make_token(lexer, (FoxyTokenType)FOXY_KEYWORD_TABLE[i].subtype, FOXY_KEYWORD_TABLE[i].category);
        }
    }

    return make_token(lexer, FOX_TOKEN_IDENTIFIER, FOXY_TOKEN_CAT_IDENTIFIER);
}

static FoxyToken scan_number(FoxyLexer *lexer) {
    while (isdigit((unsigned char)peek(lexer))) advance(lexer);

    FoxyTokenType type = FOX_TOKEN_INT_LITERAL;

    /* Parte Fraccionaria */
    if (peek(lexer) == '.' && isdigit((unsigned char)peek_next(lexer))) {
        advance(lexer); /* Consumir '.' */
        while (isdigit((unsigned char)peek(lexer))) advance(lexer);
        type = FOX_TOKEN_DOUBLE_LITERAL;
    }

    /* Escanear sufijos numéricos explícitos de Foxy-Lang (i, ui, l, ul, ll, ull, f, d, ld, n) */
    const char *suffix_start = lexer->cursor;
    if (isalpha((unsigned char)peek(lexer))) {
        while (isalpha((unsigned char)peek(lexer))) advance(lexer);
        size_t s_len = (size_t)(lexer->cursor - suffix_start);

        switch (s_len) {
            case 1:
                switch (suffix_start[0]) {
                    case 'i': type = FOX_TOKEN_INT_LITERAL;    break;
                    case 'l': type = FOX_TOKEN_LONG_LITERAL;   break;
                    case 'f': type = FOX_TOKEN_FLOAT_LITERAL;  break;
                    case 'd': type = FOX_TOKEN_DOUBLE_LITERAL; break;
                    case 'n': type = FOX_TOKEN_NUMBER_LITERAL; break;
                    default:  break;
                }
                break;

            case 2: {
                /* Empaquetar 2 caracteres en un uint16_t en Big Endian para usar switch */
                uint16_t code2 = (uint16_t)(((uint16_t)(unsigned char)suffix_start[0] << 8) |
                                            (uint16_t)(unsigned char)suffix_start[1]);
                switch (code2) {
                    case ('u' << 8 | 'i'): type = FOX_TOKEN_UINT_LITERAL;   break;
                    case ('u' << 8 | 'l'): type = FOX_TOKEN_ULONG_LITERAL;  break;
                    case ('l' << 8 | 'l'): type = FOX_TOKEN_LLONG_LITERAL;  break;
                    case ('l' << 8 | 'd'): type = FOX_TOKEN_LDOUBLE_LITERAL; break;
                    default: break;
                }
                break;
            }

            case 3: {
                /* Empaquetar 3 caracteres en un uint32_t */
                uint32_t code3 = ((uint32_t)(unsigned char)suffix_start[0] << 16) |
                                 ((uint32_t)(unsigned char)suffix_start[1] << 8)  |
                                 ((uint32_t)(unsigned char)suffix_start[2]);
                switch (code3) {
                    case ('u' << 16 | 'l' << 8 | 'l'): type = FOX_TOKEN_ULLONG_LITERAL; break;
                    default: break;
                }
                break;
            }

            default:
                break;
        }
    }

    return make_token(lexer, type, FOXY_TOKEN_CAT_LITERAL);
}

static FoxyToken scan_string(FoxyLexer *lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\\' && peek_next(lexer) != '\0') advance(lexer);
        advance(lexer);
    }

    if (is_at_end(lexer)) return error_token(lexer, "Unterminated string literal.");
    advance(lexer); /* Consumir " de cierre */

    return make_token(lexer, FOX_TOKEN_STRING_LITERAL, FOXY_TOKEN_CAT_LITERAL);
}

static FoxyToken scan_char(FoxyLexer *lexer) {
    if (peek(lexer) == '\\') advance(lexer);
    advance(lexer);

    if (!match(lexer, '\'')) return error_token(lexer, "Unterminated char literal.");
    return make_token(lexer, FOX_TOKEN_CHAR_LITERAL, FOXY_TOKEN_CAT_LITERAL);
}

static FoxyToken scan_label_or_less(FoxyLexer *lexer) {
    /* Verificar si sigue un identificador válido para una etiqueta: <nombre_etiqueta> */
    const char *temp = lexer->cursor;
    
    if (isalpha((unsigned char)*temp) || *temp == '_') {
        while (isalnum((unsigned char)*temp) || *temp == '_') {
            temp++;
        }
        /* Si termina con '>', es una etiqueta de goto */
        if (*temp == '>') {
            lexer->cursor = temp + 1; /* Consumir hasta el '>' */
            lexer->column += (uint32_t)(lexer->cursor - lexer->token_start - 1);
            return make_token(lexer, FOX_TOKEN_LABEL, FOXY_TOKEN_CAT_IDENTIFIER);
        }
    }

    /* Operadores de comparación y desplazamiento */
    if (match(lexer, '=')) {
        return make_token(lexer, FOX_TOKEN_LE, FOXY_TOKEN_CAT_OPERATOR);
    }

    if (match(lexer, '<')) {
        if (match(lexer, '=')) {
            return make_token(lexer, FOX_TOKEN_LSHIFT_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
        }
        return make_token(lexer, FOX_TOKEN_LSHIFT, FOXY_TOKEN_CAT_OPERATOR);
    }

    return make_token(lexer, FOX_TOKEN_LT, FOXY_TOKEN_CAT_OPERATOR);
}

FoxyToken f_lexer_next_token(FoxyLexer *lexer) {
    skip_whitespace_and_comments(lexer);

    lexer->token_start = lexer->cursor;

    if (is_at_end(lexer)) return make_token(lexer, FOX_TOKEN_EOF, 0);

    char c = advance(lexer);

    if (isalpha((unsigned char)c) || c == '_') return scan_identifier_or_keyword(lexer);
    if (isdigit((unsigned char)c)) return scan_number(lexer);

    switch (c) {
        case '(': return make_token(lexer, FOX_TOKEN_LPAREN, FOXY_TOKEN_CAT_OPERATOR);
        case ')': return make_token(lexer, FOX_TOKEN_RPAREN, FOXY_TOKEN_CAT_OPERATOR);
        case '{': return make_token(lexer, FOX_TOKEN_LBRACE, FOXY_TOKEN_CAT_OPERATOR);
        case '}': return make_token(lexer, FOX_TOKEN_RBRACE, FOXY_TOKEN_CAT_OPERATOR);
        case '[': return make_token(lexer, FOX_TOKEN_LBRACKET, FOXY_TOKEN_CAT_OPERATOR);
        case ']': return make_token(lexer, FOX_TOKEN_RBRACKET, FOXY_TOKEN_CAT_OPERATOR);
        case ';': return make_token(lexer, FOX_TOKEN_SEMICOLON, FOXY_TOKEN_CAT_OPERATOR);
        case ':': return make_token(lexer, FOX_TOKEN_COLON, FOXY_TOKEN_CAT_OPERATOR);
        case ',': return make_token(lexer, FOX_TOKEN_COMMA, FOXY_TOKEN_CAT_OPERATOR);
        case '?': return make_token(lexer, FOX_TOKEN_QUESTION, FOXY_TOKEN_CAT_OPERATOR);
        case '#': return make_token(lexer, FOX_TOKEN_HASH, FOXY_TOKEN_CAT_OPERATOR);
        case '~': return make_token(lexer, FOX_TOKEN_TILDE, FOXY_TOKEN_CAT_OPERATOR);
        case '^':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_XOR_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_CARET, FOXY_TOKEN_CAT_OPERATOR);

        case '"': return scan_string(lexer);
        case '\'': return scan_char(lexer);

        case '.':
            if (match(lexer, '.')) {
                if (match(lexer, '.')) return make_token(lexer, FOX_TOKEN_ELLIPSIS, FOXY_TOKEN_CAT_OPERATOR);
                return make_token(lexer, FOX_TOKEN_DOTDOT, FOXY_TOKEN_CAT_OPERATOR);
            }
            return make_token(lexer, FOX_TOKEN_DOT, FOXY_TOKEN_CAT_OPERATOR);

        case '+':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_PLUS_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '+')) return make_token(lexer, FOX_TOKEN_INC, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_PLUS, FOXY_TOKEN_CAT_OPERATOR);

        case '-':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_MINUS_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '-')) return make_token(lexer, FOX_TOKEN_DEC, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '>')) return make_token(lexer, FOX_TOKEN_ARROW, FOXY_TOKEN_CAT_OPERATOR); // Retornar ARROW si la semántica del lenguaje lo requiere
            return make_token(lexer, FOX_TOKEN_MINUS, FOXY_TOKEN_CAT_OPERATOR);

        case '*':
            if (match(lexer, '*')) {
                if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_POWER_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
                return make_token(lexer, FOX_TOKEN_POWER, FOXY_TOKEN_CAT_OPERATOR);
            }
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_STAR_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_STAR, FOXY_TOKEN_CAT_OPERATOR);

        case '/':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_SLASH_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_SLASH, FOXY_TOKEN_CAT_OPERATOR);

        case '%':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_PERCENT_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_PERCENT, FOXY_TOKEN_CAT_OPERATOR);

        case '=':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_EQ, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '>')) return make_token(lexer, FOX_TOKEN_FAT_ARROW, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);

        case '!':
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_NEQ, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_BANG, FOXY_TOKEN_CAT_OPERATOR);

        case '<':
            return scan_label_or_less(lexer);

        case '>':
            if (match(lexer, '>')) {
                if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_RSHIFT_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
                return make_token(lexer, FOX_TOKEN_RSHIFT, FOXY_TOKEN_CAT_OPERATOR);
            }
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_GE, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_GT, FOXY_TOKEN_CAT_OPERATOR);

        case '&':
            if (match(lexer, '&')) return make_token(lexer, FOX_TOKEN_AND, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_AND_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_AMPERSAND, FOXY_TOKEN_CAT_OPERATOR);

        case '|':
            if (match(lexer, '|')) return make_token(lexer, FOX_TOKEN_OR, FOXY_TOKEN_CAT_OPERATOR);
            if (match(lexer, '=')) return make_token(lexer, FOX_TOKEN_OR_ASSIGN, FOXY_TOKEN_CAT_OPERATOR);
            return make_token(lexer, FOX_TOKEN_PIPE, FOXY_TOKEN_CAT_OPERATOR);
    }

    return error_token(lexer, "Unrecognized character.");
}

FoxyToken f_lexer_peek_token(FoxyLexer *lexer) {
    FoxyLexer state_copy = *lexer;
    FoxyToken token = f_lexer_next_token(&state_copy);
    return token;
}

const char *f_lexer_token_type_to_string(FoxyTokenType type) {
#if FOXY_COMPILER_SUPPORTS_XMACROS
    switch (type) {
#define F(ftoken) case ftoken: return #ftoken;
        FOXY_TOKEN_LIST(F)
#undef F
        default: return "UNKNOWN_TOKEN";
    }
#else
    switch (type) {
        case FOX_TOKEN_EOF: return "FOX_TOKEN_EOF";
        case FOX_TOKEN_ERROR: return "FOX_TOKEN_ERROR";
        case FOX_TOKEN_IDENTIFIER: return "FOX_TOKEN_IDENTIFIER";
        case FOX_TOKEN_LABEL: return "FOX_TOKEN_LABEL";
        case FOX_TOKEN_INT_LITERAL: return "FOX_TOKEN_INT_LITERAL";
        case FOX_TOKEN_UINT_LITERAL: return "FOX_TOKEN_UINT_LITERAL";
        case FOX_TOKEN_LONG_LITERAL: return "FOX_TOKEN_LONG_LITERAL";
        case FOX_TOKEN_ULONG_LITERAL: return "FOX_TOKEN_ULONG_LITERAL";
        case FOX_TOKEN_LLONG_LITERAL: return "FOX_TOKEN_LLONG_LITERAL";
        case FOX_TOKEN_ULLONG_LITERAL: return "FOX_TOKEN_ULLONG_LITERAL";
        case FOX_TOKEN_FLOAT_LITERAL: return "FOX_TOKEN_FLOAT_LITERAL";
        case FOX_TOKEN_DOUBLE_LITERAL: return "FOX_TOKEN_DOUBLE_LITERAL";
        case FOX_TOKEN_LDOUBLE_LITERAL: return "FOX_TOKEN_LDOUBLE_LITERAL";
        case FOX_TOKEN_NUMBER_LITERAL: return "FOX_TOKEN_NUMBER_LITERAL";
        case FOX_TOKEN_CHAR_LITERAL: return "FOX_TOKEN_CHAR_LITERAL";
        case FOX_TOKEN_STRING_LITERAL: return "FOX_TOKEN_STRING_LITERAL";
        case FOX_TOKEN_ARROW: return "FOX_TOKEN_ARROW";
        case FOX_TOKEN_FAT_ARROW: return "FOX_TOKEN_FAT_ARROW";
        case FOX_TOKEN_PTR_ARROW: return "FOX_TOKEN_PTR_ARROW";
        case FOX_TOKEN_DOTDOT: return "FOX_TOKEN_DOTDOT";
        case FOX_TOKEN_ELLIPSIS: return "FOX_TOKEN_ELLIPSIS";
        default: return "FOX_TOKEN_GENERIC";
    }
#endif
}

void f_lexer_print_token(const FoxyToken *token) {
    printf("[%s:%u:%u] Cat: %u, Subtype: %u (%s), Lexema: '%.*s'\n",
           token->pos.filename, token->pos.line, token->pos.column,
           token->type_category, token->type,
           f_lexer_token_type_to_string(token->type),
           token->length, token->start);
}