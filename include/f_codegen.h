#ifndef F_CODEGEN_H
    #define F_CODEGEN_H

    #include <stdint.h>
    #include <stddef.h>
    #include "f_value.h"
    #include "f_parser.h"
    #include "f_constant.h"
    #include "f_vm.h"

    // Estructura optimizada de BytecodeBuffer:
    // Ordenada estrictamente por el sizeof de sus campos (de mayor a menor)
    // para minimizar el offset padding de alineación en arquitecturas x86_64.
    typedef struct {
        uint8_t* code;
        size_t count;
        size_t capacity;
        uint32_t error_code;

        // --- Constant Pool temporal del Codegen ---
        FoxyValue* constants;
        size_t constants_count;
        size_t constants_capacity;
        
        unsigned int is_dynamic : 1;
        unsigned int has_error : 1;
        unsigned int optimized : 1;
        unsigned int reserved : 5;
    } BytecodeBuffer;

    // Estructura para variables locales en compilación
    typedef struct {
        char name[64];
        uint8_t index;
    } FoxyLocalSymbol;

    // Contexto de generación reutilizando la estructura FoxyValue
    typedef struct {
        // 1. Buffer de Bytecode empaquetado
        uint8_t *code;
        size_t code_size;
        size_t code_capacity;

        // 2. Pool de constantes estructurado con FoxyValue (alineado con FoxyVM)
        FoxyValue *constants;
        size_t constants_count;
        size_t constants_capacity;

        // 3. Tabla de variables locales
        FoxyLocalSymbol *locals;
        size_t locals_count;
        size_t locals_capacity;
    } FoxyCodegenContext;

    // Funciones públicas del Generador de Código (Codegen):
    FoxyCodegenContext* f_codegen_create(void);
    void f_codegen_free(FoxyCodegenContext *ctx);
    void f_codegen_emit_byte(FoxyCodegenContext *ctx, uint8_t byte);
    void f_codegen_visit(FoxyCodegenContext *ctx, ASTNode *node);
    uint8_t f_codegen_add_constant(BytecodeBuffer* buffer, FoxyValueType type, const char* sval);
    uint8_t f_codegen_get_or_add_local(FoxyCodegenContext *ctx, const char *name);
    uint32_t f_codegen_add_constant_string(FoxyCodegenContext* ctx, const char* str);
    size_t f_codegen_add_chararray_const_to_buffer(BytecodeBuffer* buffer, const char* raw_str);

    // Transfiere la propiedad del constant pool hacia un destino (ej. FoxyVM) 
    // desvinculando el puntero del contexto para evitar doble liberación.
    void f_codegen_steal_constants(FoxyCodegenContext *ctx, FoxyValue **out_constants, size_t *out_count);

    // Transfiere la propiedad del bytecode y constant pool hacia FoxyVM, 
    // desvinculando los punteros del contexto para evitar "double free".
    void f_codegen_steal_resources(FoxyCodegenContext *ctx, uint8_t **out_code, size_t *out_code_size, FoxyValue **out_constants, size_t *out_constants_count);

    // Inicializa el búfer de bytecode
    void f_codegen_buffer_init(BytecodeBuffer* buffer);
    
    // Escribe un byte en el búfer con control de capacidad
    void f_codegen_buffer_write(BytecodeBuffer* buffer, uint8_t byte);
    
    // Libera la memoria del búfer de bytecode
    void f_codegen_buffer_free(BytecodeBuffer* buffer);

    // Función principal: Recorre el AST y genera el flujo de bytecode ejecutable
    uint8_t* f_generate_bytecode(ASTNode* ast_root, size_t* out_size);

    // (Temporal) Función para examinar bytecode generado en tiempo real
    void f_codegen_visit_call(FoxyCodegenContext *ctx, ASTNode *node);

#endif // F_CODEGEN_H