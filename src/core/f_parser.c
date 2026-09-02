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

ASTNode* parser_parse(FoxyLexer* lexer) {
    ASTNode* root = parser_create_node(AST_PROGRAM);
    FoxyToken current_token = f_lexer_next_token(lexer);

    while (current_token.start != NULL && *current_token.start != '\0') {
        
        // 1. Manejo de la directiva include
        if (current_token.subtype == FOXY_TOKEN_INCLUDE) {
            ASTNode* include_node = parser_create_node(AST_INCLUDE);
            include_node->token = current_token;
            
            // Siguiente token: la ruta del archivo/módulo (Literal)
            FoxyToken path_token = f_lexer_next_token(lexer);
            ASTNode* path_node = parser_create_node(AST_LITERAL);
            path_node->token = path_token;
            parser_add_child(include_node, path_node);

            parser_add_child(root, include_node);
        } 
        // 2. Manejo de Identificadores y llamadas a funciones
        else if (current_token.subtype >= FOXY_TOKEN_IDENTIFIER_NAME && 
                   current_token.subtype <= FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY) {
            
            ASTNode* call_node = parser_create_node(AST_EXPR_CALL);
            call_node->token = current_token;

            FoxyToken next = f_lexer_next_token(lexer);
            if (next.subtype == FOXY_TOKEN_OPERATOR_LPAREN) {
                // El argumento dentro de los paréntesis es un literal
                FoxyToken arg = f_lexer_next_token(lexer);
                
                ASTNode* arg_node = parser_create_node(AST_LITERAL);
                arg_node->token = arg;
                parser_add_child(call_node, arg_node);

                FoxyToken close_paren = f_lexer_next_token(lexer);
                (void)close_paren;
            }

            parser_add_child(root, call_node);
        } 
        else if (current_token.subtype == FOXY_TOKEN_ERROR_SYNTAX) {
            fprintf(stderr, "[Foxy Parser Error] Error de sintaxis detectado en el flujo.\n");
            break;
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