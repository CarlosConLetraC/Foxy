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
                int closing_match = 0;
                while (f_lexer_peek(lexer) != '\0') {
                    if (f_lexer_peek(lexer) == '@') {
                        int current_ats = 0;
                        while (f_lexer_peek(lexer) == '@') {
                            current_ats++;
                            f_lexer_advance(lexer);
                        }
                        // Si la cantidad de '@' al cerrar coincide con la cantidad inicial, el comentario termina
                        if (current_ats >= at_count) {
                            closing_match = 1;
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