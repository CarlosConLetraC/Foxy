#ifndef F_CODEGEN_H
    #define F_CODEGEN_H

    #include "f_settings.h"
    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include "f_value.h"
    #include "f_foxcode.h"
    #include "f_foxmode.h"
    #include "f_ast.h"

    #define FOXY_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

    #define FOXY_GROW_ARRAY(type, pointer, oldCount, newCount) \
        ((type*)realloc((pointer), sizeof(type) * (newCount)))

    typedef struct FoxyCodegenLocal {
        char name[FOXY_MAX_IDENTIFIER_LEN];
        uint32_t depth;
        bool is_captured;
        size_t index;
    } FoxyCodegenLocal;

    typedef struct FoxyCodegen {
        FoxInstruction *bytecode;
        size_t code_count;
        size_t code_capacity;

        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;

        FoxyCodegenLocal locals[FOXY_MAX_LOCALS];
        size_t locals_count;
        uint32_t scope_depth;

        struct FoxyCodegen *parent;
    } FoxyCodegen;

    void f_codegen_init(FoxyCodegen *cg);
    FoxyCodegen* f_codegen_create(void);
    void f_codegen_free(FoxyCodegen *cg);

    size_t f_codegen_emit(FoxyCodegen *cg, FoxInstruction inst);
    int f_codegen_add_constant(FoxyCodegen *cg, FoxyValue val);

    int f_codegen_resolve_local(FoxyCodegen *cg, const char *name);
    int f_codegen_add_local(FoxyCodegen *cg, const char *name, size_t name_len);

    void f_codegen_enter_scope(FoxyCodegen *cg);
    void f_codegen_exit_scope(FoxyCodegen *cg);

    bool f_codegen_visit(FoxyCodegen *cg, FoxyASTNode *node);

    void f_codegen_emit_env(FoxyCodegen *cg);
    void f_codegen_visit_env_create(FoxyCodegen *cg, FoxyASTNode *node);
    void f_codegen_visit_env_bind(FoxyCodegen *cg, FoxyASTNode *node);
    void f_codegen_visit_popen(FoxyCodegen *cg, FoxyASTNode *node);
    bool f_codegen_generate(FoxyCodegen *cg, FoxyASTNode *ast_root);
#endif // F_CODEGEN_H