#ifndef F_LEXER_H
    #define F_LEXER_H
    #include "f_tokens.h"
    typedef struct {
        int type_category; // Categoría principal del token
        int subtype;       // El valor específico del enum correspondiente
        const char* start; // Puntero al inicio del lexema en el código fuente
        int length;        // Longitud del lexema
        int line;          // Número de línea para manejo de errores
        int column;        // Número de columna
    } FoxyToken;

    typedef struct {
        const char* source;  // Código fuente completo de foxy-lang
        const char* current; // Puntero al carácter actual en análisis
        int line;            // Línea actual
        int column;          // Columna actual
    } FoxyLexer;

    void f_lexer_init(FoxyLexer* lexer, const char* source_code);
    void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer);
    FoxyToken f_lexer_next_token(FoxyLexer* lexer);
#endif