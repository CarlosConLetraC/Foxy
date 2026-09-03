#ifndef F_LEXER_H
    #define F_LEXER_H

    #include "f_token.h"
    #include <string.h>
    #include <ctype.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdarg.h>

    // Categorías principales para type_category
    #define FOXY_TOKEN_CAT_KEYWORD     1
    #define FOXY_TOKEN_CAT_TYPE        2
    #define FOXY_TOKEN_CAT_IDENTIFIER  3
    #define FOXY_TOKEN_CAT_OPERATOR    4
    #define FOXY_TOKEN_CAT_METHOD      5
    #define FOXY_TOKEN_CAT_ERROR       6

    typedef struct {
        const char* start;              // Puntero al lexema en el fuente
        uint32_t length;                // Longitud del lexema
        uint16_t line;                  // Línea de origen
        uint16_t column;                // Columna de origen
        unsigned int type_category : 4; // Categoría principal (0-15)
        unsigned int subtype : 12;      // Subtipo del enum correspondiente (0-4095)
        
        // Unión para almacenar valores evaluados directamente en el lexer
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
        const char* text;            // Nombre de la palabra reservada
        unsigned int category : 4;   // Categoría (1: Keyword, 2: Primitive Type)
        unsigned int subtype : 12;    // Valor enum asignado
    } KeywordMap;

    static const KeywordMap KEYWORD_TABLE[] = {
        // Categoría 1: Keywords de Estructura y Control (FOXY_TOKEN)
        { "function",  FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_FUNCTION },
        { "class",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_CLASS },
        { "from",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_FROM },
        { "struct",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_STRUCT },
        { "enum",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_ENUM },
        { "export",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_EXPORT },
        { "use",       FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_USE },
        { "overrule",  FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_OVERRULE },
        { "super",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_SUPER },
        { "self",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_SELF },
        { "for",       FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_FOR },
        { "if",        FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_IF },
        { "switch",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_SWITCH },
        { "case",      FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_CASE },
        { "default",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_DEFAULT },
        { "break",     FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_BREAK },
        { "return",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_RETURN },
        { "include",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_INCLUDE },
        { "private",   FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_PRIVATE },
        { "protected", FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_PROTECTED },
        { "public",    FOXY_TOKEN_CAT_KEYWORD, FOXY_TOKEN_PUBLIC },

        // Categoría 2: Tipos de datos primitivos (FOXY_TOKEN_TYPE)
        { "char",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_CHAR },
        { "int",       FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_INT },
        { "number",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_NUMBER },
        { "float",     FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_FLOAT },
        { "double",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_DOUBLE },
        { "long",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_LONG },
        { "object",    FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_OBJECT },
        { "dict",      FOXY_TOKEN_CAT_TYPE,    FOXY_TOKEN_TYPE_DICT }
    };

    static const size_t KEYWORD_TABLE_SIZE = sizeof(KEYWORD_TABLE) / sizeof(KeywordMap);

    // API Pública del Lexer
    void f_lexer_init(FoxyLexer* lexer, const char* source_code, const char* filename);
    void f_lexer_skip_comments_and_whitespace(FoxyLexer* lexer);
    void f_throw_error(const FoxyLexer* lexer, FoxyErrorType err_type, const char* format, ...);
    const char* f_error_type_to_string(FoxyErrorType err_type);
    FoxyToken f_lexer_next_token(FoxyLexer* lexer);
#endif // F_LEXER_H