#ifndef F_LEXER_H
#define F_LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "f_token.h"

/**
 * @brief Estado interno del analizador léxico.
 */
typedef struct {
    const char *source;
    const char *cursor;
    const char *token_start;
    const char *filename;
    uint32_t line;
    uint32_t column;
} FoxyLexer;

/* API Principal del Lexer */
void f_lexer_init(FoxyLexer *lexer, const char *source, const char *filename);
FoxyToken f_lexer_next_token(FoxyLexer *lexer);
FoxyToken f_lexer_peek_token(FoxyLexer *lexer);

/* Helpers y Diagnóstico */
const char *f_lexer_token_type_to_string(FoxyTokenType type);
void f_lexer_print_token(const FoxyToken *token);
#endif // F_LEXER_HD