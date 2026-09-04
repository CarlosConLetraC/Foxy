#ifndef F_CODEGEN_H
    #define F_CODEGEN_H

    #include "f_ast.h"
    #include "f_foxcode.h"
    #include "f_foxmode.h"

    typedef struct {
        char name[64];
        int index;
    } FoxyCodegenLocal;

    typedef struct {
        FoxyCodegenLocal locals[256];
        int local_count;
        
        FoxInstruction *bytecode;
        size_t code_count;
        size_t code_capacity;

        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;
    } FoxyCodegen;

    void f_codegen_init(FoxyCodegen *cg);
    void f_codegen_free(FoxyCodegen *cg);
    size_t f_codegen_emit(FoxyCodegen *cg, FoxInstruction inst);
    void f_codegen_emit_byte(FoxyCodegen *cg, uint8_t opcode);
    int f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val);

    bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node);
    
    // Auxiliares de emisión
    void f_codegen_emit_null(FoxyCodegen *cg);
    void f_codegen_emit_env(FoxyCodegen *cg);
    
    // Visitantes de concurrencia y entornos
    void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node);
    void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node);
    void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node);

    bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root);
#endif // F_CODEGEN_H