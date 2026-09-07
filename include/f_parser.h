#ifndef F_PARSER_H
    #define F_PARSER_H

    #include "f_lexer.h"
    #include "f_ast.h"

    typedef struct {
        FoxyLexer *lexer;
        FoxyToken current_token;
        FoxyToken peek_token;
        bool had_error;
    } FoxyParser;

    char* f_parser_token_to_string(FoxyToken *token);
    FoxyASTNode* f_parser_parse(FoxyLexer *lexer);
#endif // F_PARSER_H