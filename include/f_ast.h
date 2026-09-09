#ifndef F_AST_H
    #define F_AST_H

    #include "f_settings.h"
    #include <stddef.h>
    #include <stdbool.h>
    #include "f_value.h"

    typedef struct FoxyVM FoxyVM;

    #define FOXY_AST_NODE_LIST(F) \
        F(FOXY_AST_NODE_PROGRAM,        "PROGRAM") \
        F(FOXY_AST_NODE_INCLUDE,        "INCLUDE") \
        F(FOXY_AST_NODE_EXPR_STMT,      "EXPR_STMT") \
        F(FOXY_AST_NODE_CALL,           "CALL") \
        F(FOXY_AST_NODE_LITERAL,        "LITERAL") \
        F(FOXY_AST_NODE_IDENTIFIER,     "IDENTIFIER") \
        F(FOXY_AST_NODE_BLOCK,          "BLOCK") \
        F(FOXY_AST_NODE_BINARY_OP,      "BINARY_OP") \
        F(FOXY_AST_NODE_VAR_DECL,       "VAR_DECL") \
        F(FOXY_AST_NODE_ASSIGN,         "ASSIGN") \
        F(FOXY_AST_NODE_IF,             "IF") \
        F(FOXY_AST_NODE_WHILE,          "WHILE") \
        F(FOXY_AST_NODE_RETURN,         "RETURN") \
        F(FOXY_AST_NODE_FUNCTION,       "FUNCTION") \
        F(FOXY_AST_NODE_FOR,            "FOR") \
        F(FOXY_AST_NODE_ENV,            "ENV") \
        F(FOXY_AST_NODE_ENV_CREATE,     "ENV_CREATE") \
        F(FOXY_AST_NODE_ENV_BIND,       "ENV_BIND") \
        F(FOXY_AST_NODE_POPEN,          "POPEN") \
        F(FOXY_AST_NODE_MEMBER_ACCESS,  "MEMBER_ACCESS") \
        F(FOXY_AST_NODE_INDEX_ACCESS,   "INDEX_ACCESS")

    typedef enum __attribute__((__packed__)) {
    #define F(node_type, name_str) node_type,
        FOXY_AST_NODE_LIST(F)
    #undef F
        AST_NODE_COUNT
    } FoxyASTNodeType;

    extern const char * const FOXY_AST_NODE_TYPE_NAMES[];
    typedef struct FoxyASTNode FoxyASTNode;

    struct FoxyASTNode {
        FoxyASTNodeType type;
        
        union {
            struct { char *path; } include_node;

            struct {
                char *callee_name;
                FoxyASTNode **arguments;
                size_t arg_count;
                size_t arg_capacity;
            } call_node;

            struct { FoxyValue value; } literal_node;

            struct {
                FoxyASTNode **statements;
                size_t count;
                size_t capacity;
            } block_node;

            struct { char *name; } identifier_node;

            struct {
                FoxyASTNode *left;
                FoxyASTNode *right;
                int op_token;
            } binary_node;

            struct {
                FoxyASTNode **statements;
                size_t count;
                size_t capacity;
            } program_node;

            struct {
                char *name;
                FoxyASTNode *initializer;
            } var_decl_node;

            struct {
                char *name;
                FoxyASTNode *value;
            } assign_node;

            struct {
                FoxyASTNode *condition;
                FoxyASTNode *then_branch;
                FoxyASTNode *else_branch;
            } if_node;

            struct {
                FoxyASTNode *condition;
                FoxyASTNode *body;
            } while_node;

            struct {
                char *name;
                char **param_names;
                size_t param_count;
                FoxyASTNode *body;
            } function_node;

            struct {
                FoxyASTNode *init;
                FoxyASTNode *condition;
                FoxyASTNode *increment;
                FoxyASTNode *body;
            } for_node;

            struct { FoxyASTNode *value; } return_node;

            struct { FoxyASTNode *name_expr; } env_create_node;

            struct {
                FoxyASTNode *process_expr;
                FoxyASTNode *env_expr;
            } env_bind_node;

            struct {
                FoxyASTNode *callback_expr;
                FoxyASTNode *name_expr;
                FoxyASTNode *env_expr;
            } popen_node;

            struct {
                FoxyASTNode *target;
                char *field;
            } member_access_node;

            struct {
                FoxyASTNode *target;
                FoxyASTNode *index;
            } index_access_node;

            struct { FoxyASTNode *expression; } expr_stmt_node;
        } as;
    };

    void f_ast_node_free(FoxyASTNode *node, FoxyVM *vm);
    void f_ast_program_add(FoxyASTNode *program, FoxyASTNode *stmt);
    void f_ast_call_add_arg(FoxyASTNode *call_node, FoxyASTNode *arg);
    FoxyASTNode* f_ast_node_new(FoxyASTNodeType type);
    FoxyASTNode* f_ast_create_program(void);
    FoxyASTNode* f_ast_create_include(const char *path);
    FoxyASTNode* f_ast_create_call(const char *callee);
    FoxyASTNode* f_ast_create_literal(FoxyValue val);
    FoxyASTNode* f_ast_create_identifier(const char *name);
    FoxyASTNode* f_ast_create_binary_op(int op_token, FoxyASTNode *left, FoxyASTNode *right);
    FoxyASTNode* f_ast_create_var_decl(const char *name, FoxyASTNode *initializer);
    FoxyASTNode* f_ast_create_assign(FoxyASTNode *left, FoxyASTNode *right, FoxyVM *vm);
    FoxyASTNode* f_ast_create_if(FoxyASTNode *condition, FoxyASTNode *then_branch, FoxyASTNode *else_branch);
    FoxyASTNode* f_ast_create_while(FoxyASTNode *condition, FoxyASTNode *body);
    FoxyASTNode* f_ast_create_function(const char *name, FoxyASTNode *body);
    FoxyASTNode* f_ast_create_for(FoxyASTNode *init, FoxyASTNode *condition, FoxyASTNode *increment, FoxyASTNode *body);
    FoxyASTNode* f_ast_create_return(FoxyASTNode *value);
    FoxyASTNode* f_ast_create_expr_stmt(FoxyASTNode *expr);
    FoxyASTNode* f_ast_create_env(void);
    FoxyASTNode* f_ast_create_env_create(FoxyASTNode *name_expr);
    FoxyASTNode* f_ast_create_env_bind(FoxyASTNode *proc_expr, FoxyASTNode *env_expr);
    FoxyASTNode* f_ast_create_popen(FoxyASTNode *callback_expr, FoxyASTNode *name_expr, FoxyASTNode *env_expr);

    const char* f_ast_node_type_to_string(FoxyASTNodeType type);    
#endif // F_AST_H