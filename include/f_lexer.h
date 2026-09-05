#ifndef F_LEXER_H
    #define F_LEXER_H

    #include "f_token.h"
    #include <string.h>
    #include <ctype.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdarg.h>

    typedef struct {
        const char* source;
        const char* current;
        const char* filename;
        uint32_t line;
        uint32_t column;
    } FoxyLexer;

    // API Pública del Lexer
    void f_lexer_init(FoxyLexer* lexer, const char* source_code, const char* filename);
    void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer);
    void f_lexer_throw_error(const FoxyLexer* lexer, FoxyErrorType err_type, const char* format, ...);
    const char* f_lexer_error_type_to_string(FoxyErrorType err_type);
    FoxyToken f_lexer_next_token(FoxyLexer* lexer);
#endif // F_LEXER_H