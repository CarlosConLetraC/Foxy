#include "f_lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* ========================================================================= */
/* FUNCIONES AUXILIARES DE LECTURA                                           */
/* ========================================================================= */

static inline bool is_at_end(FoxyLexer *lexer) {
    return *lexer->cursor == '\0';
}

static inline char peek(FoxyLexer *lexer) {
    return *lexer->cursor;
}

static inline char peek_next(FoxyLexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->cursor[1];
}

static inline char advance(FoxyLexer *lexer) {
    char c = *lexer->cursor++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static inline bool match(FoxyLexer *lexer, char expected) {
    if (is_at_end(lexer) || *lexer->cursor != expected) return false;
    advance(lexer);
    return true;
}

static FoxyToken make_token(FoxyLexer *lexer, const char *start, uint16_t cat, uint16_t sub) {
    FoxyToken token;
    token.start = start;
    token.length = (uint32_t)(lexer->cursor - start);
    token.type_category = cat;
    token.subtype = sub;
    token.loc.filename = lexer->filename;
    token.loc.line = lexer->line;
    token.loc.column = lexer->column - token.length;
    return token;
}

static FoxyToken error_token(FoxyLexer *lexer, const char *msg) {
    FoxyToken token;
    token.start = msg;
    token.length = (uint32_t)strlen(msg);
    token.type_category = FOXY_TOKEN_CAT_ERROR;
    token.subtype = 0;
    token.loc.filename = lexer->filename;
    token.loc.line = lexer->line;
    token.loc.column = lexer->column;
    return token;
}

/* ========================================================================= */
/* IGNORAR ESPACIOS Y COMENTARIOS                                            */
/* ========================================================================= */

static void skip_whitespace_and_comments(FoxyLexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance(lexer);
                break;

            case '@': {
                // Contar cuántas '@' consecutivas forman la secuencia de apertura
                size_t at_count = 0;
                while (peek(lexer) == '@') {
                    at_count++;
                    advance(lexer);
                }

                if (at_count == 1) {
                    // Comentario de una sola línea: @ ... \n
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else {
                    // Comentario de bloque con N arrobas (@@ ... @@, @@@ ... @@@, etc.)
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '@') {
                            // Contar cuántas '@' consecutivas hay en el cierre
                            size_t close_count = 0;
                            while (peek(lexer) == '@') {
                                close_count++;
                                advance(lexer);
                            }

                            // Si coincide la cantidad N de '@', se cierra el bloque
                            if (close_count == at_count) {
                                break;
                            }
                            // Si no coincide la cantidad, continúa consumiendo el comentario
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

/* ========================================================================= */
/* RECONOCIMIENTO DE IDENTIFICADORES, PALABRAS CLAVE Y METAMÉTODOS           */
/* ========================================================================= */

static FoxyToken identifier_or_keyword(FoxyLexer *lexer, const char *start) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    uint32_t length = (uint32_t)(lexer->cursor - start);

    // Buscar en la tabla estática de palabras clave, tipos y metamétodos
    for (size_t i = 0; i < FOXY_KEYWORD_TABLE_SIZE; i++) {
        if (strlen(FOXY_KEYWORD_TABLE[i].text) == length &&
            strncmp(start, FOXY_KEYWORD_TABLE[i].text, length) == 0) {
            return make_token(lexer, start, FOXY_KEYWORD_TABLE[i].category, FOXY_KEYWORD_TABLE[i].subtype);
        }
    }

    // Si no está en la tabla, es un identificador estándar
    return make_token(lexer, start, FOXY_TOKEN_CAT_IDENTIFIER, 0);
}

/* ========================================================================= */
/* RECONOCIMIENTO DE NÚMEROS Y NÚMEROS SUFIJADOS                             */
/* ========================================================================= */

static FoxyToken number_literal(FoxyLexer *lexer, const char *start) {
    bool is_float = false;

    while (isdigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        is_float = true;
        advance(lexer); // Consume '.'
        while (isdigit(peek(lexer))) advance(lexer);
    }

    // Sufijo de tipo de literal procesado mediante switch-case
    switch (peek(lexer)) {
        case 'f': case 'F':
            advance(lexer);
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_FLOAT_F);

        case 'd': case 'D':
            advance(lexer);
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_DOUBLE_D);

        case 'i': case 'I':
            advance(lexer);
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_INT_I);

        case 'u': case 'U': {
            advance(lexer);
            char next = peek(lexer);
            if (next == 'i' || next == 'I') {
                advance(lexer);
                return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_UINT_UI);
            } else if (next == 'l' || next == 'L') {
                advance(lexer);
                if (peek(lexer) == 'l' || peek(lexer) == 'L') {
                    advance(lexer);
                    return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_ULLONG_ULL);
                }
                return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_ULONG_UL);
            }
            // Si solo encuentra 'u', cae al default o maneja int/uint
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_UINT_UI);
        }

        case 'l': case 'L': {
            advance(lexer);
            char next = peek(lexer);
            if (next == 'l' || next == 'L') {
                advance(lexer);
                return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_LLONG_LL);
            } else if (next == 'd' || next == 'D') {
                advance(lexer);
                return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_LDOUBLE_LD);
            }
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_LONG_L);
        }

        case 'n': case 'N':
            advance(lexer);
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_NUMBER_N);

        default:
            if (is_float) {
                return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_DOUBLE_D);
            }
            return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_INT_I);
    }
}

/* ========================================================================= */
/* RECONOCIMIENTO DE CADENAS LITERALES                                       */
/* ========================================================================= */

static FoxyToken string_literal(FoxyLexer *lexer, const char *start) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\\' && peek_next(lexer) != '\0') {
            advance(lexer); // Escapar carácter
        }
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Cadena no cerrada.");
    }

    advance(lexer); // Consume el '"' de cierre
    return make_token(lexer, start, FOXY_TOKEN_CAT_LITERAL, FOXY_TOKEN_LIT_STRING);
}

/* ========================================================================= */
/* INICIALIZACIÓN Y BUCLE PRINCIPAL DEL LEXER                                */
/* ========================================================================= */

void foxy_lexer_init(FoxyLexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->cursor = source;
    lexer->filename = filename;
    lexer->line = 1;
    lexer->column = 1;
}

FoxyToken foxy_lexer_next_token(FoxyLexer *lexer) {
    skip_whitespace_and_comments(lexer);

    if (is_at_end(lexer)) {
        return make_token(lexer, lexer->cursor, 0, 0); // EOF
    }

    const char *start = lexer->cursor;
    char c = advance(lexer);

    // Identificadores y palabras clave (incluye metamétodos `__*`)
    if (isalpha(c) || c == '_') {
        return identifier_or_keyword(lexer, start);
    }

    // Literales numéricos
    if (isdigit(c)) {
        return number_literal(lexer, start);
    }

    // Cadenas de texto
    if (c == '"') {
        return string_literal(lexer, start);
    }

    // Operadores y Delimitadores (Cruciales para llaves de dict complejas `[...]`)
    switch (c) {
        case '(': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LPAREN);
        case ')': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_RPAREN);
        case '{': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LBRACE);
        case '}': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_RBRACE);
        case '[': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LBRACKET);
        case ']': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_RBRACKET);
        case ',': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_COMMA);
        case ';': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_SEMICOLON);
        case ':': return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_COLON);
        
        case '.':
            if (peek(lexer) == '.' && peek_next(lexer) == '.') {
                advance(lexer);
                advance(lexer);
                return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_ELLIPSIS);
            }
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_DOT);

        case '=':
            if (match(lexer, '>')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LAMBDA);
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_EQ);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_ASSIGN);

        case '+':
            if (match(lexer, '+')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_INC);
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_ADD_ASSIGN);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_ADD);

        case '-':
            if (match(lexer, '>')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_ARROW);
            if (match(lexer, '-')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_DEC);
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_SUB_ASSIGN);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_SUB);

        case '*':
            if (match(lexer, '*')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_POW);
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_MUL_ASSIGN);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_MUL);

        case '/':
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_DIV_ASSIGN);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_DIV);

        case '!':
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_NEQ);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_NOT);

        case '<':
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LE);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_LT);

        case '>':
            if (match(lexer, '=')) return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_GE);
            return make_token(lexer, start, FOXY_TOKEN_CAT_OPERATOR, FOXY_TOKEN_OPERATOR_GT);
    }

    return error_token(lexer, "Carácter no reconocido.");
}