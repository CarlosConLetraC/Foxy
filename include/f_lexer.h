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
        const char* start;              // Puntero de 64 bits (alineación base)
        uint32_t length;                // 4 bytes
        uint16_t line;                  // 2 bytes
        uint16_t column;                // 2 bytes
        unsigned int type_category : 4; // Campo de bits (0-15)
        unsigned int subtype : 12;      // Campo de bits (0-4095)
        
        // Unión para almacenar valores directos opcionales según el tipo de token
        union {
            int64_t int_val;
            double float_val;
            void* custom_data;
        } as;
    } FoxyToken;

    typedef struct {
        const char* source;
        const char* current;
        const char* filename;
        uint32_t line;
        uint32_t column;
    } FoxyLexer;

    typedef struct {
        const char* text;            // Puntero de 64 bits
        unsigned int category : 4;   // Empaquetado con campos de bits
        unsigned int subtype : 12;
    } KeywordMap;

    static const KeywordMap KEYWORD_TABLE[] = {
        // Categoría 1: Keywords (FOXY_TOKEN)
        { "function",  1, FOXY_TOKEN_FUNCTION },
        { "class",     1, FOXY_TOKEN_CLASS },
        { "from",      1, FOXY_TOKEN_FROM },
        { "struct",    1, FOXY_TOKEN_STRUCT },
        { "enum",      1, FOXY_TOKEN_ENUM },
        { "export",    1, FOXY_TOKEN_EXPORT },
        { "use",       1, FOXY_TOKEN_USE },
        { "overrule",  1, FOXY_TOKEN_OVERRULE },
        { "super",     1, FOXY_TOKEN_SUPER },
        { "self",      1, FOXY_TOKEN_SELF },
        { "for",       1, FOXY_TOKEN_FOR },
        { "if",        1, FOXY_TOKEN_IF },
        { "switch",    1, FOXY_TOKEN_SWITCH },
        { "case",      1, FOXY_TOKEN_CASE },
        { "default",   1, FOXY_TOKEN_DEFAULT },
        { "break",     1, FOXY_TOKEN_BREAK },
        { "return",    1, FOXY_TOKEN_RETURN },
        { "include",   1, FOXY_TOKEN_INCLUDE },
        { "private",   1, FOXY_TOKEN_PRIVATE },
        { "protected", 1, FOXY_TOKEN_PROTECTED },
        { "public",    1, FOXY_TOKEN_PUBLIC },

        // Categoría 2: Tipos de datos (FOXY_TOKEN_TYPE)
        { "char",                 2, FOXY_TOKEN_TYPE_CHAR },
        { "int",                  2, FOXY_TOKEN_TYPE_INT },
        { "number",               2, FOXY_TOKEN_TYPE_NUMBER },
        { "float",                2, FOXY_TOKEN_TYPE_FLOAT },
        { "double",               2, FOXY_TOKEN_TYPE_DOUBLE },
        { "long",                 2, FOXY_TOKEN_TYPE_LONG },
        { "long long",            2, FOXY_TOKEN_TYPE_LONG_LONG },
        { "unsigned long long",   2, FOXY_TOKEN_TYPE_UNSIGNED_LONG_LONG },
        { "object",               2, FOXY_TOKEN_TYPE_OBJECT },
        { "class",                2, FOXY_TOKEN_TYPE_CLASS },
        { "dict",                 2, FOXY_TOKEN_TYPE_DICT },
        { "struct",               2, FOXY_TOKEN_TYPE_STRUCT },
        { "function",             2, FOXY_TOKEN_TYPE_FUNCTION }
    };

    static const int KEYWORD_TABLE_SIZE = sizeof(KEYWORD_TABLE) / sizeof(KeywordMap);

    // Funciones públicas del lexer
    void f_lexer_init(FoxyLexer* lexer, const char* source_code, const char* filename);
    void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer);
    void f_throw_error(const FoxyLexer* lexer, FOXY_TOKEN_ERROR err_type, const char* format, ...);
    const char* f_error_type_to_string(FOXY_TOKEN_ERROR err_type);
    FoxyToken f_lexer_next_token(FoxyLexer* lexer);
#endif // F_LEXER_H