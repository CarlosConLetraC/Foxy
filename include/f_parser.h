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

    // --- Prototipos de gestión del AST ---
    FoxyASTNode* f_ast_node_new(FoxyASTNodeType type);
    FoxyASTNode* f_parser_parse(FoxyLexer *lexer);
    void f_ast_node_free(FoxyASTNode *node);

    // Constructores auxiliares
    FoxyASTNode* f_ast_create_program(void);
    void f_ast_program_add(FoxyASTNode *program, FoxyASTNode *stmt);
    FoxyASTNode* f_ast_create_include(const char *path);
    FoxyASTNode* f_ast_create_call(const char *callee);
    void f_ast_call_add_arg(FoxyASTNode *call_node, FoxyASTNode *arg);
    FoxyASTNode* f_ast_create_literal(FoxyValue val);
    FoxyASTNode* f_ast_create_identifier(const char *name);
    
    // Firmas sincronizadas con la implementación y el parser:
    FoxyASTNode* f_ast_create_binary_op(int op_token, FoxyASTNode *left, FoxyASTNode *right);
    FoxyASTNode* f_ast_create_var_decl(const char *name, FoxyASTNode *initializer);
    FoxyASTNode* f_ast_create_assign(FoxyASTNode *left, FoxyASTNode *right);
    
    FoxyASTNode* f_ast_create_if(FoxyASTNode *condition, FoxyASTNode *then_branch, FoxyASTNode *else_branch);
    FoxyASTNode* f_ast_create_while(FoxyASTNode *condition, FoxyASTNode *body);
    FoxyASTNode* f_ast_create_for(FoxyASTNode *init, FoxyASTNode *condition, FoxyASTNode *increment, FoxyASTNode *body);
    FoxyASTNode* f_ast_create_return(FoxyASTNode *value);
    FoxyASTNode* f_ast_create_expr_stmt(FoxyASTNode *expr);
#endif // F_PARSER_H