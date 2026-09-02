#ifndef F_CODEGEN_H
    #define F_CODEGEN_H

    #include <stdint.h>
    #include <stddef.h>
    #include "f_parser.h"
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
        FoxyConstant* constants;
        size_t constants_count;
        size_t constants_capacity;
        
        unsigned int is_dynamic : 1;
        unsigned int has_error : 1;
        unsigned int optimized : 1;
        unsigned int reserved : 5;
    } BytecodeBuffer;

    // Funciones públicas del Generador de Código (Codegen)
    
    // Inicializa el búfer de bytecode
    void f_codegen_buffer_init(BytecodeBuffer* buffer);
    
    // Escribe un byte en el búfer con control de capacidad
    void f_codegen_buffer_write(BytecodeBuffer* buffer, uint8_t byte);
    
    // Libera la memoria del búfer de bytecode
    void f_codegen_buffer_free(BytecodeBuffer* buffer);

    // Función principal: Recorre el AST y genera el flujo de bytecode ejecutable
    uint8_t* f_generate_bytecode(ASTNode* ast_root, size_t* out_size);

#endif // F_CODEGEN_H