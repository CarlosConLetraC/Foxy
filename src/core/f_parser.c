#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_parser.h"
#include "f_token.h"

static ASTNode* parser_create_node(ASTNodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "[Foxy Parser Error] Fallo de asignación de memoria para ASTNode.\n");
        exit(1);
    }
    node->type = type;
    node->child_count = 0;
    node->capacity = 4;
    node->children = (ASTNode**)malloc(sizeof(ASTNode*) * node->capacity);
    memset(&node->token, 0, sizeof(FoxyToken));
    return node;
}

static void parser_add_child(ASTNode* parent, ASTNode* child) {
    if (parent->child_count >= parent->capacity) {
        parent->capacity *= 2;
        ASTNode** temp = realloc(parent->children, sizeof(ASTNode*) * parent->capacity);
        if (!temp) {
            fprintf(stderr, "[Foxy Parser Error] Fallo de reasignación de memoria para hijos del AST.\n");
            exit(1);
        }
        parent->children = temp;
    }
    parent->children[parent->child_count++] = child;
}

ASTNode* f_parser_create_node(ASTNodeType type, FoxyToken token) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) return NULL;

    node->type = type;
    node->token = token;
    node->child_count = 0;
    node->capacity = 4;
    node->children = (ASTNode**)malloc(sizeof(ASTNode*) * node->capacity);
    
    if (!node->children) {
        free(node);
        return NULL;
    }
    return node;
}

ASTNode* f_parser_parse(FoxyLexer* lexer) {
    ASTNode* root = parser_create_node(AST_PROGRAM);
    FoxyToken current_token = f_lexer_next_token(lexer);

    while (current_token.start != NULL && *current_token.start != '\0') {
        
        // 1. Directiva include
        if (current_token.subtype == FOXY_TOKEN_INCLUDE) {
            ASTNode* include_node = parser_create_node(AST_INCLUDE);
            include_node->token = current_token;
            
            FoxyToken path_token = f_lexer_next_token(lexer);
            ASTNode* path_node = parser_create_node(AST_LITERAL);
            path_node->token = path_token;
            parser_add_child(include_node, path_node);

            parser_add_child(root, include_node);
        } 
        // 2. Declaración de Variables / Tipos (ej. int a = 40, char mensaje[] = "...")
        else if (current_token.subtype >= FOXY_TOKEN_IDENTIFIER_NAME && 
                   current_token.subtype <= FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY) {
            
            FoxyToken type_token = current_token;
            FoxyToken var_name_token = f_lexer_next_token(lexer);

            ASTNode* var_decl_node = parser_create_node(AST_VAR_DECL);
            var_decl_node->token = type_token;

            ASTNode* name_node = parser_create_node(AST_LITERAL);
            name_node->token = var_name_token;
            parser_add_child(var_decl_node, name_node);

            FoxyToken next = f_lexer_next_token(lexer);

            // Manejo de corchetes en arreglos []
            if (next.subtype == FOXY_TOKEN_OPERATOR_LBRACKET || (next.start && *next.start == '[')) {
                FoxyToken close_bracket = f_lexer_next_token(lexer);
                (void)close_bracket;
                next = f_lexer_next_token(lexer);
            }

            // Asignación '='
            if (next.subtype == FOXY_TOKEN_OPERATOR_ASSIGN || (next.start && *next.start == '=')) {
                FoxyToken val_token = f_lexer_next_token(lexer);
                ASTNode* val_node = parser_create_node(AST_LITERAL);
                val_node->token = val_token;
                parser_add_child(var_decl_node, val_node);
                
                // Consumir tokens adicionales de la expresión si los hay (ej. operadores aritméticos en línea)
                FoxyToken lookahead = f_lexer_next_token(lexer);
                while (lookahead.start && *lookahead.start != '\n' && *lookahead.start != '\r' && *lookahead.start != '\0') {
                    lookahead = f_lexer_next_token(lexer);
                }
            }

            parser_add_child(root, var_decl_node);
        }
        // 3. Declaración de Funciones
        else if (current_token.subtype == FOXY_TOKEN_FUNCTION || 
                   (current_token.length == 8 && strncmp(current_token.start, "function", 8) == 0)) {
            
            ASTNode* func_node = parser_create_node(AST_FUNCTION_DEF);
            
            FoxyToken func_name = f_lexer_next_token(lexer);
            func_node->token = func_name;

            // Avanzar hasta la llave de apertura '{'
            FoxyToken t = f_lexer_next_token(lexer);
            while (t.start && *t.start != '{' && *t.start != '\0') {
                t = f_lexer_next_token(lexer);
            }

            // Procesar cuerpo de la función hasta la llave de cierre '}'
            FoxyToken body_token = f_lexer_next_token(lexer);
            while (body_token.start && *body_token.start != '}' && *body_token.start != '\0') {
                if (body_token.length == 6 && strncmp(body_token.start, "return", 6) == 0) {
                    ASTNode* ret_node = parser_create_node(AST_RETURN_STMT);
                    FoxyToken ret_val = f_lexer_next_token(lexer);
                    ASTNode* val_node = parser_create_node(AST_LITERAL);
                    val_node->token = ret_val;
                    parser_add_child(ret_node, val_node);
                    parser_add_child(func_node, ret_node);
                }
                body_token = f_lexer_next_token(lexer);
            }

            parser_add_child(root, func_node);
        }
        // 4. Llamadas a funciones sueltas (ej. println, print)
        else if (current_token.subtype == FOXY_TOKEN_IDENTIFIER_NAME || 
                   (current_token.start && ((*current_token.start >= 'a' && *current_token.start <= 'z') || 
                                            (*current_token.start >= 'A' && *current_token.start <= 'Z')))) {
            
            FoxyToken start_token = current_token;
            ASTNode* current_expr = parser_create_node(AST_LITERAL); // O un nodo base de identificador
            current_expr->token = start_token;

            FoxyToken next = f_lexer_next_token(lexer);
            
            // Soportar encadenamiento de puntos (ej. sys -> .out -> .println)
            while (next.subtype == FOXY_TOKEN_OPERATOR_DOT || (next.start && *next.start == '.')) {
                FoxyToken member_token = f_lexer_next_token(lexer); // Nombre del miembro (ej. "out" o "println")
                
                ASTNode* member_node = parser_create_node(AST_MEMBER_ACCESS);
                member_node->token = member_token;
                parser_add_child(member_node, current_expr); // El objeto anterior es el padre/base
                
                current_expr = member_node;
                next = f_lexer_next_token(lexer);
            }

            // Si después del identificador o miembro viene un paréntesis '(', es una llamada
            if (next.subtype == FOXY_TOKEN_OPERATOR_LPAREN || (next.start && *next.start == '(')) {
                ASTNode* call_node = parser_create_node(AST_EXPR_CALL);
                call_node->token = current_expr->token;
                
                // Si la expresión previa fue un miembro (ej. sys.out), la guardamos o reubicamos
                // Dependiendo de cómo lo maneje tu codegen, podemos adjuntar current_expr como hijo o referencia
                parser_add_child(call_node, current_expr);

                // Procesar argumentos separados por comas hasta el ')'
                FoxyToken arg_token = f_lexer_next_token(lexer);
                while (arg_token.start && *arg_token.start != ')' && *arg_token.start != '\0') {
                    if (*arg_token.start != ',') { // Ignorar comas separadoras
                        ASTNode* arg_node = parser_create_node(AST_LITERAL);
                        arg_node->token = arg_token;
                        parser_add_child(call_node, arg_node);
                    }
                    arg_token = f_lexer_next_token(lexer);
                }

                parser_add_child(root, call_node);
            } else {
                parser_add_child(root, current_expr);
            }
        }

        current_token = f_lexer_next_token(lexer);
    }

    return root;
}

void free_ast(ASTNode* node) {
    if (!node) return;
    for (uint32_t i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    free(node->children);
    free(node);
}

void f_parser_add_child(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return;

    if (parent->child_count >= parent->capacity) {
        parent->capacity *= 2;
        ASTNode** temp = (ASTNode**)realloc(parent->children, sizeof(ASTNode*) * parent->capacity);
        if (!temp) return;
        parent->children = temp;
    }
    parent->children[parent->child_count++] = child;
}