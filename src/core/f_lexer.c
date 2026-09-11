#include "f_settings.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "f_lexer.h"

static FoxyToken make_token(FoxyLexer *lexer, uint16_t category, uint16_t type);
static FoxyToken error_token(FoxyLexer *lexer, const char *message);
static void skip_whitespace_and_comments(FoxyLexer *lexer);
static FoxyToken scan_identifier_or_keyword(FoxyLexer *lexer);
static FoxyToken scan_number(FoxyLexer *lexer);
static FoxyToken scan_string(FoxyLexer *lexer);
static FoxyToken scan_char(FoxyLexer *lexer);
static FoxyToken scan_parenthesized_modifier_or_paren(FoxyLexer *lexer);
static FoxyToken scan_label_or_lt(FoxyLexer *lexer);

static inline bool is_at_end(FoxyLexer *lexer) {
    return *lexer->cursor == '\0';
}

static inline char advance(FoxyLexer *lexer) {
    lexer->cursor++;
    lexer->column++;
    return lexer->cursor[-1];
}

static inline char peek(FoxyLexer *lexer) {
    return *lexer->cursor;
}

static inline char peek_next(FoxyLexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->cursor[1];
}

static bool match(FoxyLexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->cursor != expected) return false;
    lexer->cursor++;
    lexer->column++;
    return true;
}

void f_lexer_init(FoxyLexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->cursor = source;
    lexer->token_start = source;
    lexer->filename = filename ? filename : "<stdin>";
    lexer->line = 1;
    lexer->column = 1;
}

static FoxyToken make_token(FoxyLexer *lexer, uint16_t category, uint16_t type) {
    FoxyToken token;
    token.type_category = category;
    token.type = type;
    token.start = lexer->token_start;
    token.length = (uint32_t)(lexer->cursor - lexer->token_start);
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column - token.length;
    return token;
}

static FoxyToken error_token(FoxyLexer *lexer, const char *message) {
    FoxyToken token;
    token.type_category = FOXY_TOKEN_CAT_ERROR;
    token.type = FOX_TOKEN_ERROR;
    token.start = message;
    token.length = (uint32_t)strlen(message);
    token.pos.filename = lexer->filename;
    token.pos.line = lexer->line;
    token.pos.column = lexer->column;
    return token;
}

static void skip_whitespace_and_comments(FoxyLexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 0;
                advance(lexer);
                break;
            case '@': {
                size_t at_count = 0;
                while (peek(lexer) == '@') {
                    at_count++;
                    advance(lexer);
                }

                if (at_count == 1) {
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                } else {
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                            advance(lexer);
                            continue;
                        }
                        
                        if (peek(lexer) == '@') {
                            size_t close_count = 0;
                            while (peek(lexer) == '@') {
                                close_count++;
                                advance(lexer);
                            }
                            if (close_count == at_count) break;
                        } else {
                            advance(lexer);
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

static FoxyToken scan_parenthesized_modifier_or_paren(FoxyLexer *lexer) {
    if (peek(lexer) == 'p') {
        if (strncmp(lexer->cursor, "private)", 8) == 0) {
            lexer->cursor += 8;
            lexer->column += 8;
            return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PRIVATE);
        }
        if (strncmp(lexer->cursor, "protected)", 10) == 0) {
            lexer->cursor += 10;
            lexer->column += 10;
            return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PROTECTED);
        }
        if (strncmp(lexer->cursor, "public)", 7) == 0) {
            lexer->cursor += 7;
            lexer->column += 7;
            return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_MOD_PUBLIC);
        }
    }

    return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LPAREN);
}

static FoxyToken scan_label_or_lt(FoxyLexer *lexer) {
    if (isalpha(peek(lexer)) || peek(lexer) == '_') {
        const char *scan = lexer->cursor;
        while (isalnum(*scan) || *scan == '_') scan++;
        if (*scan == '>') {
            while (peek(lexer) != '>') advance(lexer);
            advance(lexer);
            return make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_LABEL);
        }
    }
    
    if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LE);
    if (match(lexer, '<')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LSHIFT);
    return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LT);
}

static FoxyToken scan_identifier_or_keyword(FoxyLexer *lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);

    size_t length = lexer->cursor - lexer->token_start;
    const char *text = lexer->token_start;

    switch (length) {
        case 2:
            if (memcmp(text, "if", 2) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_IF);
            break;
        case 3:
            if (memcmp(text, "for", 3) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOR);
            if (memcmp(text, "use", 3) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_USE);
            if (memcmp(text, "try", 3) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TRY);
            if (memcmp(text, "int", 3) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_INT);
            break;
        case 4:
            if (memcmp(text, "null", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_NULL);
            if (memcmp(text, "true", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TRUE);
            if (memcmp(text, "bool", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_BOOL);
            if (memcmp(text, "char", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_CHAR);
            if (memcmp(text, "uint", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_UINT);
            if (memcmp(text, "long", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_LONG);
            if (memcmp(text, "else", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSE);
            if (memcmp(text, "case", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CASE);
            if (memcmp(text, "goto", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_GOTO);
            if (memcmp(text, "from", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FROM);
            if (memcmp(text, "dict", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_DICT);
            if (memcmp(text, "enum", 4) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ENUM);
            break;
        case 5:
            if (memcmp(text, "false", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FALSE);
            if (memcmp(text, "uchar", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_UCHAR);
            if (memcmp(text, "short", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_SHORT);
            if (memcmp(text, "ulong", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_ULONG);
            if (memcmp(text, "llong", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_LLONG);
            if (memcmp(text, "float", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_FLOAT);
            if (memcmp(text, "class", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CLASS);
            if (memcmp(text, "super", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SUPER);
            if (memcmp(text, "catch", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CATCH);
            if (memcmp(text, "final", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FINAL);
            if (memcmp(text, "break", 5) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_BREAK);
            break;
        case 6:
            if (memcmp(text, "elseif", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ELSEIF);
            if (memcmp(text, "ushort", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_USHORT);
            if (memcmp(text, "ullong", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_ULLONG);
            if (memcmp(text, "double", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_DOUBLE);
            if (memcmp(text, "number", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_NUMBER);
            if (memcmp(text, "struct", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_STRUCT);
            if (memcmp(text, "return", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_RETURN);
            if (memcmp(text, "switch", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_SWITCH);
            if (memcmp(text, "except", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXCEPT);
            if (memcmp(text, "export", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_EXPORT);
            if (memcmp(text, "typeof", 6) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_TYPEOF);
            break;
        case 7:
            if (memcmp(text, "ldouble", 7) == 0) return make_token(lexer, FOXY_TOKEN_CAT_TYPE, FOX_TOKEN_KW_LDOUBLE);
            if (memcmp(text, "foreach", 7) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FOREACH);
            if (memcmp(text, "default", 7) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DEFAULT);
            if (memcmp(text, "include", 7) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_INCLUDE);
            break;
        case 8:
            if (memcmp(text, "function", 8) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_FUNCTION);
            if (memcmp(text, "overrule", 8) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_OVERRULE);
            if (memcmp(text, "continue", 8) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_CONTINUE);
            break;
        case 10:
            if (memcmp(text, "ancestorof", 10) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_ANCESTOROF);
            break;
        case 12:
            if (memcmp(text, "descendantof", 12) == 0) return make_token(lexer, FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_KW_DESCENDANTOF);
            break;
    }

    return make_token(lexer, FOXY_TOKEN_CAT_IDENTIFIER, FOX_TOKEN_IDENTIFIER);
}

static FoxyToken scan_number(FoxyLexer *lexer) {
    bool is_float = false;

    while (isdigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        is_float = true;
        advance(lexer);
        while (isdigit(peek(lexer))) advance(lexer);
    }

    if (isalpha(peek(lexer))) {
        // Acumular hasta 3 caracteres del sufijo en un entero de 32 bits (little-endian)
        uint32_t tag = 0;
        int shift = 0;

        while (isalpha(peek(lexer)) && shift < 24) {
            char ch = (char)(advance(lexer) | 0x20); // Normalización a minúscula bitwise
            tag |= ((uint32_t)ch << shift);
            shift += 8;
        }

#if USE_COMPUTED_GOTO
        /* ==================================================================
         * OPTIMIZACIÓN HIGH-PERFORMANCE: Computed Goto (GCC / Clang)
         * ================================================================== */
        static const void *const suffix_dispatch[256] = {
            ['f'] = &&L_float,
            ['d'] = &&L_double,
            ['i'] = &&L_int,
            ['n'] = &&L_number,
            ['l'] = &&L_long,
            ['u'] = &&L_uint,
        };

        // Para sufijos de 1 solo carácter (los más frecuentes), el salto es O(1) directo por tabla
        if ((tag & 0xFFFFFF00) == 0) {
            uint8_t key = (uint8_t)tag;
            if (suffix_dispatch[key]) {
                goto *suffix_dispatch[key];
            }
            goto L_default;
        }

        // Despacho directo para sufijos multicarácter mediante etiquetas
        if (tag == ('u' | ('i' << 8))) goto L_uint;
        if (tag == ('u' | ('l' << 8))) goto L_ulong;
        if (tag == ('l' | ('d' << 8))) goto L_ldouble;
        if (tag == ('l' | ('l' << 8))) goto L_llong;
        if (tag == ('u' | ('l' << 8) | ('l' << 16))) goto L_ullong;

        goto L_default;

    L_float:   return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_FLOAT_LITERAL);
    L_double:  return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_DOUBLE_LITERAL);
    L_int:     return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_INT_LITERAL);
    L_number:  return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_NUMBER_LITERAL);
    L_long:    return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LONG_LITERAL);
    L_uint:    return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_UINT_LITERAL);
    L_ulong:   return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_ULONG_LITERAL);
    L_ldouble: return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LDOUBLE_LITERAL);
    L_llong:   return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LLONG_LITERAL);
    L_ullong:  return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_ULLONG_LITERAL);
    L_default: return error_token(lexer, "Unrecognized numeric literal suffix.");

#else
        /* ==================================================================
         * FALLBACK PORTABLE: Switch-Case Plano (MSVC / ANSI C)
         * ================================================================== */
        switch (tag) {
            case 'f':
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_FLOAT_LITERAL);
            case 'd':
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_DOUBLE_LITERAL);
            case 'i':
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_INT_LITERAL);
            case 'n':
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_NUMBER_LITERAL);
            case 'l':
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LONG_LITERAL);
            case 'u':
            case 'u' | ('i' << 8):
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_UINT_LITERAL);
            case 'u' | ('l' << 8):
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_ULONG_LITERAL);
            case 'l' | ('d' << 8):
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LDOUBLE_LITERAL);
            case 'l' | ('l' << 8):
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_LLONG_LITERAL);
            case 'u' | ('l' << 8) | ('l' << 16):
                return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_ULLONG_LITERAL);
            default:
                return error_token(lexer, "Unrecognized numeric literal suffix.");
        }
#endif
    }

    return is_float ? make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_DOUBLE_LITERAL) 
                    : make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_INT_LITERAL);
}

static FoxyToken scan_string(FoxyLexer *lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        if (peek(lexer) == '\\' && peek_next(lexer) != '\0') advance(lexer);
        advance(lexer);
    }

    if (is_at_end(lexer)) return error_token(lexer, "Unterminated string literal.");
    advance(lexer);
    return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_STRING_LITERAL);
}

static FoxyToken scan_char(FoxyLexer *lexer) {
    if (peek(lexer) == '\\') advance(lexer);
    advance(lexer);
    if (peek(lexer) != '\'') return error_token(lexer, "Unterminated char literal.");
    advance(lexer);
    return make_token(lexer, FOXY_TOKEN_CAT_LITERAL, FOX_TOKEN_CHAR_LITERAL);
}

FoxyToken f_lexer_next_token(FoxyLexer *lexer) {
    skip_whitespace_and_comments(lexer);
    lexer->token_start = lexer->cursor;

    if (is_at_end(lexer)) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_EOF);

    char c = advance(lexer);

    if (isalpha(c) || c == '_') return scan_identifier_or_keyword(lexer);
    if (isdigit(c)) return scan_number(lexer);

    switch (c) {
        case '(': return scan_parenthesized_modifier_or_paren(lexer);
        case ')': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RPAREN);
        case '{': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LBRACE);
        case '}': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RBRACE);
        case '[': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_LBRACKET);
        case ']': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RBRACKET);
        case ';': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SEMICOLON);
        case ':': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COLON);
        case ',': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_COMMA);
        case '?': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_QUESTION);
        case '~': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_TILDE);
        case '^': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_CARET);
        case '#': return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_HASH);

        case '.':
            if (peek(lexer) == '.' && peek_next(lexer) == '.') {
                advance(lexer); advance(lexer);
                return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ELLIPSIS);
            }
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DOT);

        case '=':
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_EQ);
            if (match(lexer, '>')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ARROW);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_ASSIGN);

        case '!':
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, match(lexer, '=') ? FOX_TOKEN_NEQ : FOX_TOKEN_BANG);

        case '+':
            if (match(lexer, '+')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_INC);
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PLUS_ASSIGN);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PLUS);

        case '-':
            if (match(lexer, '-')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_DEC);
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_MINUS_ASSIGN);
            if (match(lexer, '>')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PTR_ARROW);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_MINUS);

        case '*':
            if (match(lexer, '*')) {
                if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_POWER_ASSIGN);
                return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_POWER);
            }
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_STAR_ASSIGN);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_STAR);

        case '/':
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SLASH_ASSIGN);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_SLASH);

        case '%':
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PERCENT_ASSIGN);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PERCENT);

        case '&':
            if (match(lexer, '&')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_AND);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_AMPERSAND);

        case '|':
            if (match(lexer, '|')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_OR);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_PIPE);

        case '<': return scan_label_or_lt(lexer);

        case '>':
            if (match(lexer, '=')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_GE);
            if (match(lexer, '>')) return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_RSHIFT);
            return make_token(lexer, FOXY_TOKEN_CAT_OPERATOR, FOX_TOKEN_GT);

        case '"': return scan_string(lexer);
        case '\'': return scan_char(lexer);
    }

    return error_token(lexer, "Unexpected character.");
}

FoxyToken f_lexer_peek_token(FoxyLexer *lexer) {
    FoxyLexer state_copy = *lexer;
    return f_lexer_next_token(&state_copy);
}