#include "f_lexer.h"

// Funciones auxiliares internas de apoyo (lookahead simple)
static char f_lexer_peek(FoxyLexer* lexer) {
    return *lexer->current;
}

static char f_lexer_advance(FoxyLexer* lexer) {
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

const char* f_error_type_to_string(FOXY_TOKEN_ERROR err_type) {
    switch (err_type) {
        #define X(name) case name: return #name;
        FOXY_TOKEN_ERROR_LIST(X)
        #undef X
    }
    return "FOXY_ERROR_UNKNOWN";
}

void f_throw_error(const FoxyLexer* lexer, FOXY_TOKEN_ERROR err_type, const char* format, ...) {
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

// Lógica principal para saltar y consumir comentarios basados en '@'
void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer) {
    for (;;) {
        char c = f_lexer_peek(lexer);
        
        // 1. Ignorar espacios en blanco estándar
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n')
            f_lexer_advance(lexer);

            // 2. Detectar inicio de comentario con '@'
        else if (c == '@') {
            int at_count = 0;
            
            // Contar cuántos '@' consecutivos se abren (ej. @, @@, @@@)
            const char* temp = lexer->current;
            while (*temp == '@') {
                at_count++;
                temp++;
            }
            
            // Consumir todos los '@' del inicio del comentario
            for (int i = 0; i < at_count; i++)
                f_lexer_advance(lexer);
            
            // Caso A: Comentario de una sola línea (exactamente un '@' y luego texto/espacio, 
            // pero validando que si es multilínea tiene más o se maneja por conteo)
            if (at_count == 1 && f_lexer_peek(lexer) != '@' && f_lexer_peek(lexer) != '\n' && f_lexer_peek(lexer) != '\0') {
                // Si el siguiente carácter no es otro '@', es un comentario de línea simple que llega hasta el salto de línea
                while (f_lexer_peek(lexer) != '\n' && f_lexer_peek(lexer) != '\0')
                    f_lexer_advance(lexer);
            } 
            // Caso B: Comentario multilínea dinámico (requiere cerrar exactamente con la misma cantidad de '@')
            else {
                // int closing_match = 0;
                while (f_lexer_peek(lexer) != '\0') {
                    if (f_lexer_peek(lexer) == '@') {
                        int current_ats = 0;
                        while (f_lexer_peek(lexer) == '@') {
                            current_ats++;
                            f_lexer_advance(lexer);
                        }
                        // Si la cantidad de '@' al cerrar coincide con la cantidad inicial, el comentario termina
                        if (current_ats >= at_count) {
                            // closing_match = 1;
                            break;
                        }
                    } else {
                        f_lexer_advance(lexer);
                    }
                }
                // Si llegara al fin de archivo sin cerrar el comentario, el lexer podría reportar un error léxico
            }
        } else {
            // Si no es espacio ni comentario, rompemos el ciclo para procesar el token real
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
    int start_line = lexer->line;
    int start_col = lexer->column;

    if (*lexer->current == '\0') {
        FoxyToken token = {0};
        token.start = start_ptr;
        token.length = 0;
        token.line = start_line;
        token.column = start_col;
        return token;
    }

    char c = f_lexer_advance(lexer);

    // 1. Identificadores, Palabras Clave y Tipos
    if (isalpha(c) || c == '_') {
        while (isalnum(f_lexer_peek(lexer)) || f_lexer_peek(lexer) == '_')
            f_lexer_advance(lexer);
        int length = (int)(lexer->current - start_ptr);

        // Buscar en la tabla de palabras clave y tipos
        int found_category = 3; // Por defecto: Identificador (FOXY_TOKEN_IDENTIFIER_NAME)
        int found_subtype = FOXY_TOKEN_IDENTIFIER_NAME;

        for (int i = 0; i < KEYWORD_TABLE_SIZE; i++) {
            if ((int)strlen(KEYWORD_TABLE[i].text) == length &&
                memcmp(start_ptr, KEYWORD_TABLE[i].text, length) == 0) {
                found_category = KEYWORD_TABLE[i].category;
                found_subtype = KEYWORD_TABLE[i].subtype;
                break;
            }
        }

        FoxyToken token;
        token.type_category = found_category;
        token.subtype = found_subtype;
        token.start = start_ptr;
        token.length = length;
        token.line = start_line;
        token.column = start_col;
        return token;
    }

    // 2. Literales Numéricos
    if (isdigit(c)) { // Usamos 'c' que ya consumió el primer dígito
        // Consumir parte entera restante
        while (isdigit(*lexer->current)) {
            lexer->current++;
            lexer->column++;
        }
        
        // Consumir parte decimal si existe
        if (*lexer->current == '.' && isdigit(*(lexer->current + 1))) {
            lexer->current++; // pasar el '.'
            lexer->column++;
            while (isdigit(*lexer->current)) {
                lexer->current++;
                lexer->column++;
            }
        }
        
        // Validación de identificadores mal formados (ej. 0hola)
        if (isalpha(*lexer->current) || *lexer->current == '_') {
            while (isalnum(*lexer->current) || *lexer->current == '_') {
                lexer->current++;
                lexer->column++;
            }
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Identificador inválido: los identificadores no pueden comenzar con números.");
        }

        // Construir el token numérico correctamente
        FoxyToken token;
        token.type_category = 4; // Categoría Literales
        token.subtype = 2;       // Subtipo numérico acorde a tu estructura
        token.start = start_ptr;
        token.length = (int)(lexer->current - start_ptr);
        token.line = start_line;
        token.column = start_col;
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
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "Fin de archivo inesperado. String sin cerrar (abierto en línea %d, columna %d)", start_line, start_col);
        }

        // Consumir la comilla de cierre
        lexer->current++;
        lexer->column++;

        FoxyToken token;
        token.type_category = 4; 
        token.subtype = 9;       
        token.start = start_ptr;
        token.length = (int)(lexer->current - start_ptr);
        token.line = start_line;
        token.column = start_col;
        return token;
    }

    // 4. Literales de Carácter ('...')
    if (c == '\'') {
        while (f_lexer_peek(lexer) != '\'' && f_lexer_peek(lexer) != '\0') {
            f_lexer_advance(lexer);
        }
        if (f_lexer_peek(lexer) == '\'') {
            f_lexer_advance(lexer);
        }

        FoxyToken token;
        token.type_category = 4;
        token.subtype = FOXY_TOKEN_IDENTIFIER_CHAR;
        token.start = start_ptr;
        token.length = (int)(lexer->current - start_ptr);
        token.line = start_line;
        token.column = start_col;
        return token;
    }

    // 5. Operadores y Delimitadores
    FoxyToken token;
    token.type_category = 5; // Categoría Operadores
    token.start = start_ptr;
    token.line = start_line;
    token.column = start_col;

    switch (c) {
        case '=': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_EQ, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_ASSIGN, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '!': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_NEQ, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_NOT, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '<': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_LE, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            if (f_lexer_peek(lexer) == '<') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_SHL, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_LT, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '>': 
            if (f_lexer_peek(lexer) == '=') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_GE, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            if (f_lexer_peek(lexer) == '>') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_SHR, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_GT, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '+': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_ADD, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '-': 
            if (f_lexer_peek(lexer) == '>') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_ARROW, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_SUB, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '*': 
            if (f_lexer_peek(lexer) == '*') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_POW, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_MUL, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '/': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_DIV, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '.': 
            if (f_lexer_peek(lexer) == '.' && lexer->current[1] == '.') {
                f_lexer_advance(lexer);
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_ELLIPSIS, .start = start_ptr, .length = 3, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_DOT, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '^': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_BXOR, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '~': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_BNOT, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '&': 
            if (f_lexer_peek(lexer) == '&') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_AND, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_BAND, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};

        case '|': 
            if (f_lexer_peek(lexer) == '|') {
                f_lexer_advance(lexer);
                return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_OR, .start = start_ptr, .length = 2, .line = start_line, .column = start_col};
            }
            return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_BOR, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        
        case '#': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_HASH, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '?': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_QUESTION, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case ':': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_COLON, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case ';': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_SEMICOLON, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case ',': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_COMMA, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '%': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_DIV, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        
        // Delimitadores con retorno directo exacto
        case '(': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_LPAREN, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case ')': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_RPAREN, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '{': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_LBRACE, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '}': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_RBRACE, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case '[': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_LBRACKET, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        case ']': return (FoxyToken){.type_category = 5, .subtype = FOXY_TOKEN_OPERATOR_RBRACKET, .start = start_ptr, .length = 1, .line = start_line, .column = start_col};
        
        default: {
            char err_msg[64];
            snprintf(err_msg, sizeof(err_msg), "Carácter inesperado '%c'", c);
            f_throw_error(lexer, FOXY_TOKEN_ERROR_SYNTAX, "%s", err_msg);
        }
        return (FoxyToken){0};
    }

    token.length = (int)(lexer->current - start_ptr);
    return token;
}