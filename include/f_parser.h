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

    // Prototipo principal actualizado que devuelve FoxyASTNode*
    FoxyASTNode* f_parser_parse(FoxyLexer *lexer);

    // Funciones auxiliares del parser (usando el tipo de token/operador correcto de tu lexer)
    void f_parser_advance(FoxyParser *parser);
    bool f_parser_check(FoxyParser *parser, int token_type); // O el tipo de enum exacto que maneje tu FoxyToken
    bool f_parser_match(FoxyParser *parser, int token_type);
    bool f_parser_expect(FoxyParser *parser, int token_type, const char *message);

    // Análisis de expresiones y sub-reglas
    FoxyASTNode* f_parser_parse_expression(FoxyParser *parser);
    FoxyASTNode* f_parser_parse_env(FoxyParser *parser);
    FoxyASTNode* f_parser_parse_env_create(FoxyParser *parser);
    FoxyASTNode* f_parser_parse_env_bind(FoxyParser *parser);
    FoxyASTNode* f_parser_parse_popen(FoxyParser *parser);
    FoxyASTNode* f_ast_create_identifier(const char *name);
    FoxyASTNode* f_ast_create_binary_op(int op_token, FoxyASTNode *left, FoxyASTNode *right);
    FoxyASTNode* f_ast_create_assign(FoxyASTNode *left, FoxyASTNode *right);
#endif // F_PARSER_H