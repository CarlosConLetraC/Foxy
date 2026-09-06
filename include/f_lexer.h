#ifndef F_LEXER_H
    #define F_LEXER_H

    #include "f_settings.h"
    #include <stddef.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include "f_token.h"

    /**
    * Estructura de control del Lexer optimizada con bit fields e 
    * integración directa sobre f_token.h.
    */
    typedef struct {
        const char *source;     // Puntero base al código fuente
        const char *current;    // Puntero actual de lectura en el buffer
        const char *filename;   // Nombre del archivo fuente para trazabilidad de errores
        uint32_t line;          // Contador de línea actual
        uint32_t column;        // Contador de columna actual
        
        // Bloque de campos de bits empaquetados para control de estado local
        uint has_error        : 1;
        uint in_string        : 1;
        uint in_comment       : 1;
        uint escape_sequence  : 1;
        uint reserved         : 4;
    } FoxyLexer;

    // Prototipos del Ciclo de Vida y Utilidades:
    void f_lexer_init(FoxyLexer* lexer, const char* source_code, const char* filename);
    FoxyToken f_lexer_next_token(FoxyLexer* lexer);
    void f_lexer_throw_error(const FoxyLexer* lexer, FoxyErrorType err_type, const char* format, ...);
    const char* f_lexer_error_type_to_string(FoxyErrorType err_type);
#endif // F_LEXER_H