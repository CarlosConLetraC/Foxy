#include <stdbool.h>
#include "f_lexer.h"

#define F(code, name) name,
static const char* FOXY_TOKEN_ERROR_STRINGS[] = {
    FOXY_TOKEN_ERROR_LIST(F)
};
#undef F

// Auxiliares internos de lectura
static inline char f_lexer_peek(FoxyLexer* lexer) {
    return *lexer->current;
}

static inline char f_lexer_advance(FoxyLexer* lexer) {
    char c = *lexer->current;
    if (c != '\0') {
        lexer->current++;
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
    return c;
}

const char* f_error_type_to_string(FoxyErrorType err_type) {
    if (err_type < FOXY_TOKEN_ERROR_COUNT)
        return FOXY_TOKEN_ERROR_STRINGS[err_type];
    return "FOXY_ERROR_UNKNOWN";
}

void f_throw_error(const FoxyLexer* lexer, FoxyErrorType err_type, const char* format, ...) {
    const char* err_name = f_error_type_to_string(err_type);
    
    fprintf(stderr, "\033[1;31m%s\033[0m [\033[1;36m%s\033[0m:%u:%u]: ",
            err_name,
            lexer ? lexer->filename : "<unknown>",
            lexer ? lexer->line : 0,
            lexer ? lexer->column : 0);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

// Lógica de comentarios basados en '@' y espacios en blanco
void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer) {
    for (;;) {
        char c = f_lexer_peek(lexer);
        
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            f_lexer_advance(lexer);
        } else if (c == '@') {
            int at_count = 0;
            const char* temp = lexer->current;
            
            while (*temp == '@') {
                at_count++;
                temp++;
            }
            
            for (int i = 0; i < at_count; i++) {
                f_lexer_advance(lexer);
            }
            
            // Caso A: Comentario de una sola línea (exactamente un '@')
            if (at_count == 1 && f_lexer_peek(lexer) != '@' && f_lexer_peek(lexer) != '\n' && f_lexer_peek(lexer) != '\0') {
                while (f_lexer_peek(lexer) != '\n' && f_lexer_peek(lexer) != '\0') {
                    f_lexer_advance(lexer);
                }
            } 
            // Caso B: Comentario multilínea dinámico (@... @, @@... @@, etc.)
            else {
                while (f_lexer_peek(lexer) != '\0') {
                    if (f_lexer_peek(lexer) == '@') {
                        int current_ats = 0;
                        while (f_lexer_peek(lexer) == '@') {
                            current_ats++;
                            f_lexer_advance(lexer);
                        }
                        if (current_ats >= at_count) {
                            break;
                        }
                    } else {
                        f_lexer_advance(lexer);
                    }
                }
            }
        } else {
            break;
        }
    }
}

void f_lexer_init(FoxyLexer* lexer, const char* source_code, const char* filename) {
    lexer->source = source_code;
    lexer->current = source_code;
    lexer->filename = filename ? filename : "<unknown>";
    lexer->line = 1;
    lexer->column = 1;
}

FoxyToken f_lexer_next_token(FoxyLexer* lexer) {
    f_lexer_skip_comments_and_whitespace(lexer);

    const char* start_ptr = lexer->current;
    uint32_t start_line = lexer->line;
    uint32_t start_col = lexer->column;

    if (*lexer->current == '\0') {
        FoxyToken token = {0};
        token.start = start_ptr;
        token.length = 0;
        token.line = (uint16_t)start_line;
        token.column = (uint16_t)start_col;
        return token;
    }

    char c = f_lexer_advance(lexer);

    // 1. Identificadores, Palabras Clave y Tipos
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_') {
            f_lexer_advance(lexer);
        }
        
        uint32_t length = (uint32_t)(lexer->current - start_ptr);

        unsigned int found_category = FOXY_TOKEN_CAT_IDENTIFIER;
        unsigned int found_subtype = FOXY_TOKEN_IDENTIFIER_NAME;

        for (size_t i = 0; i < KEYWORD_TABLE_SIZE; i++) {
            if (strlen(KEYWORD_TABLE[i].text) == length &&
                memcmp(start_ptr, KEYWORD_TABLE[i].text, length) == 0) {
                found_category = KEYWORD_TABLE[i].category;
                found_subtype = KEYWORD_TABLE[i].subtype;
                break;
            }
        }

        FoxyToken token = {0};
        token.type_category = found_category;
        token.subtype = found_subtype;
        token.start = start_ptr;
        token.length = length;
        token.line = (uint16_t)start_line;
        token.column = (uint16_t)start_col;
        return token;
    }

    // 2. Literales Numéricos
    if (isdigit((unsigned char)c)) {
        bool is_float = false;

        while (isdigit((unsigned char)*lexer->current)) {
            lexer->current++;
            lexer->column++;
        }
        
        if (*lexer->current == '.' && isdigit((unsigned char)*(lexer->current + 1))) {
            is_float = true;
            lexer->current++;
            lexer->column++;
            while (isdigit((unsigned char)*lexer->current)) {
                lexer->current++;
                lexer->column++;
            }
        }
        
        if (isalpha((unsigned char)*lexer->current) || *lexer->current == '_') {
            while (isalnum((unsigned char)*lexer->current) || *lexer->current == '_') {
                lexer->current++;
                lexer->column++;
            }
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Identificador inválido: los identificadores no pueden comenzar con números.");
        }

        FoxyToken token = {0};
        token.type_category = FOXY_TOKEN_CAT_IDENTIFIER;
        token.start = start_ptr;
        token.length = (uint32_t)(lexer->current - start_ptr);
        token.line = (uint16_t)start_line;
        token.column = (uint16_t)start_col;

        if (is_float) {
            token.subtype = FOXY_TOKEN_IDENTIFIER_DOUBLE;
            token.as.float_val = strtod(start_ptr, NULL);
        } else {
            token.subtype = FOXY_TOKEN_IDENTIFIER_INT;
            token.as.int_val = strtoll(start_ptr, NULL, 10);
        }

        return token;
    }

    // 3. Literales de Cadena ("...")
    if (c == '"') {
        while (*lexer->current != '\0' && *lexer->current != '"') {
            if (*lexer->current == '\n') {
                f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "String sin cerrar (salto de línea inesperado)");
            }

            if (*lexer->current == '\\' && *(lexer->current + 1) != '\0') {
                lexer->current++;
                lexer->column++;
            }

            lexer->current++;
            lexer->column++;
        }

        if (*lexer->current == '\0') {
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Fin de archivo inesperado. String sin cerrar (abierto en línea %u, columna %u)", start_line, start_col);
        }

        lexer->current++;
        lexer->column++;

        FoxyToken token = {0};
        token.type_category = FOXY_TOKEN_CAT_IDENTIFIER; 
        token.subtype = FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY;       
        token.start = start_ptr;
        token.length = (uint32_t)(lexer->current - start_ptr);
        token.line = (uint16_t)start_line;
        token.column = (uint16_t)start_col;
        return token;
    }

    // 4. Literales de Carácter ('...')
    if (c == '\'') {
        while (f_lexer_peek(lexer) != '\'' && f_lexer_peek(lexer) != '\0') {
            if (f_lexer_peek(lexer) == '\\') {
                f_lexer_advance(lexer);
            }
            f_lexer_advance(lexer);
        }

        if (f_lexer_peek(lexer) == '\'') {
            f_lexer_advance(lexer);
        } else {
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Literal de carácter sin cerrar.");
        }

        FoxyToken token = {0};
        token.type_category = FOXY_TOKEN_CAT_IDENTIFIER;
        token.subtype = FOXY_TOKEN_IDENTIFIER_CHAR;
        token.start = start_ptr;
        token.length = (uint32_t)(lexer->current - start_ptr);
        token.line = (uint16_t)start_line;
        token.column = (uint16_t)start_col;
        return token;
    }

    // 5. Operadores y Delimitadores (Categoría 4)
    switch (c) {
        case '=': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_EQ, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_ASSIGN, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '!': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_NEQ, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_NOT, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '<': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_LE, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            if (f_lexer_peek(lexer) == '<') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_SHL, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_LT, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '>': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_GE, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            if (f_lexer_peek(lexer) == '>') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_SHR, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_GT, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '+': 
            if (f_lexer_peek(lexer) == '+') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_INC, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_ADD, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '-': 
            if (f_lexer_peek(lexer) == '>') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_ARROW, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_SUB, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '*': 
            if (f_lexer_peek(lexer) == '*') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_POW, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_MUL, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '/': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_DIV, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '.': 
            if (f_lexer_peek(lexer) == '.' && lexer->current[1] == '.') {
                f_lexer_advance(lexer);
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_ELLIPSIS, .start = start_ptr, .length = 3, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_DOT, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '^': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_BXOR, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '~': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_BNOT, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '&': 
            if (f_lexer_peek(lexer) == '&') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_AND, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_BAND, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '|': 
            if (f_lexer_peek(lexer) == '|') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_OR, .start = start_ptr, .length = 2, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
            }
            return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_BOR, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '#': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_HASH, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '?': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_QUESTION, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case ':': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_COLON, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case ';': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_SEMICOLON, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case ',': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_COMMA, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '%': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_DIV, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        case '(': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_LPAREN, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case ')': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_RPAREN, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '{': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_LBRACE, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '}': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_RBRACE, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case '[': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_LBRACKET, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};
        case ']': return (FoxyToken){.type_category = FOXY_TOKEN_CAT_OPERATOR, .subtype = FOXY_TOKEN_OPERATOR_RBRACKET, .start = start_ptr, .length = 1, .line = (uint16_t)start_line, .column = (uint16_t)start_col};

        default:
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Carácter inesperado '%c'", c);
            return (FoxyToken){0};
    }
}