#ifndef FOXY_LEXER_H
#define FOXY_LEXER_H

#include "f_token.h"

typedef struct {
    const char *source;         // Puntero al inicio del código fuente
    const char *cursor;         // Puntero actual en la lectura
    const char *filename;       // Nombre del archivo fuente
    uint32_t line;              // Línea actual
    uint32_t column;            // Columna actual
} FoxyLexer;

// Inicializa el lexer con un buffer de texto
void foxy_lexer_init(FoxyLexer *lexer, const char *source, const char *filename);

// Lee y retorna el siguiente token del buffer
FoxyToken foxy_lexer_next_token(FoxyLexer *lexer);
#endif // FOXY_LEXER_H