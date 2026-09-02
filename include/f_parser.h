#ifndef F_PARSER_H
    #define F_PARSER_H

    #include "f_lexer.h"
    #include <stdint.h>

    // 1. Lista maestra para tipos de nodos del AST (X-Macros)
    #define AST_NODE_TYPE_LIST(F) \
        F(AST_PROGRAM) \
        F(AST_VAR_DECL) \
        F(AST_FUNCTION_DEF) \
        F(AST_CLASS_DEF) \
        F(AST_IF_STMT) \
        F(AST_FOR_STMT) \
        F(AST_RETURN_STMT) \
        F(AST_EXPR_CALL) \
        F(AST_ASSIGNMENT) \
        F(AST_INCLUDE) \
        F(AST_LITERAL) \
        F(AST_MEMBER_ACCESS)

    // 2. Generación del Enum compacto (__attribute__((__packed__)))
    #define F(name) name,

    typedef enum __attribute__((__packed__)) {
        AST_NODE_TYPE_LIST(F)
    } ASTNodeType;

    #undef F

    // 3. Estructura optimizada del nodo del AST (ASTNode)
    typedef struct ASTNode {
        struct ASTNode** children;      // Puntero de 64 bits (alineación base de mayor tamaño)
        FoxyToken token;                // Token asociado optimizado
        uint32_t child_count;           // 4 bytes[cite: 3]
        uint32_t capacity;              // 4 bytes[cite: 3]
        ASTNodeType type;               // 1 byte (optimizado con __packed__)[cite: 3]
    } ASTNode;

    // 4. Funciones principales del análisis sintáctico
    ASTNode* f_parser_create_node(ASTNodeType type, FoxyToken token);
    ASTNode* f_parser_parse(FoxyLexer* lexer); // Actualizado a FoxyLexer para coherencia
    void free_ast(ASTNode* node);            //[cite: 3]
    void f_parser_add_child(ASTNode* parent, ASTNode* child);
#endif // F_PARSER_H