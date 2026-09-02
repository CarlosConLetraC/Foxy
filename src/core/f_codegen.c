#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_vm.h"
#include "f_codegen.h"
#include "f_foxcode.h"

static void compile_node(ASTNode* node, BytecodeBuffer* buffer);

void f_codegen_buffer_init(BytecodeBuffer* buffer) {
    if (!buffer) return;
    buffer->count = 0;
    buffer->capacity = 64;
    buffer->code = (uint8_t*)malloc(buffer->capacity);
    if (!buffer->code) {
        fprintf(stderr, "[Foxy Codegen Error] Fallo de asignación de memoria para el búfer de bytecode.\n");
        exit(1);
    }

    // --- Inicializar el Constant Pool temporal ---
    buffer->constants = NULL;
    buffer->constants_count = 0;
    buffer->constants_capacity = 0;

    buffer->error_code = 0;
    buffer->is_dynamic = 1;
    buffer->has_error  = 0;
    buffer->optimized  = 0;
    buffer->reserved   = 0;
}

void f_codegen_buffer_write(BytecodeBuffer* buffer, uint8_t byte) {
    if (!buffer || buffer->has_error) return;

    if (buffer->count >= buffer->capacity) {
        buffer->capacity *= 2;
        uint8_t* temp = (uint8_t*)realloc(buffer->code, buffer->capacity);
        if (!temp) {
            fprintf(stderr, "[Foxy Codegen Error] Fallo de reasignación de memoria en búfer de bytecode.\n");
            buffer->has_error = 1;
            buffer->error_code = 1;
            return;
        }
        buffer->code = temp;
    }
    buffer->code[buffer->count++] = byte;
}

/*
    **`strdup`:** Como cada ruta de librería o literal de texto se copió usando `strdup` en el pool temporal,
    cada `sval` tiene su propia asignación de memoria independiente que debe liberarse
    de manera individual con `free()` antes de descartar el contenedor principal.
*/
void f_codegen_buffer_free(BytecodeBuffer* buffer) {
    if (!buffer) return;

    // 1. Liberar el búfer de bytecode principal
    if (buffer->code && buffer->is_dynamic) {
        free(buffer->code);
        buffer->code = NULL;
    }

    // 2. Liberar el Constant Pool temporal y sus cadenas internas
    if (buffer->constants) {
        for (size_t i = 0; i < buffer->constants_count; i++) {
            // Si la constante es un arreglo de caracteres dinámico, liberamos su sval
            if (buffer->constants[i].type == FOXY_CONSTANT_CHARARRAY) {
                free(buffer->constants[i].as.sval);
                buffer->constants[i].as.sval = NULL;
            }
        }
        // Liberar el bloque de memoria del arreglo de constantes
        free(buffer->constants);
        buffer->constants = NULL;
    }

    // 3. Reiniciar contadores y capacidades
    buffer->count = 0;
    buffer->capacity = 0;
    buffer->constants_count = 0;
    buffer->constants_capacity = 0;
}

uint8_t* f_generate_bytecode(ASTNode* ast_root, size_t* out_size) {
    BytecodeBuffer buffer;
    f_codegen_buffer_init(&buffer);

    // 1. Compilar el AST y poblar el buffer de código y constantes
    compile_node(ast_root, &buffer);

    // 2. Calcular el tamaño total del blob binario empaquetado:
    // [num_consts (4 bytes)] + [Constantes...] + [code_size (4 bytes)] + [Bytecode de instrucciones]
    size_t constants_payload_size = sizeof(uint32_t);
    for (uint32_t i = 0; i < buffer.constants_count; i++) {
        // 1 byte de tipo + 2 bytes de longitud + longitud de la cadena
        constants_payload_size += 1 + sizeof(uint16_t) + strlen(buffer.constants[i].as.sval);
    }

    size_t total_size = constants_payload_size + sizeof(uint32_t) + buffer.count;
    uint8_t* blob = malloc(total_size);
    if (!blob) return NULL;

    size_t offset = 0;

    // 3. Escribir número de constantes
    uint32_t num_consts = (uint32_t)buffer.constants_count;
    memcpy(blob + offset, &num_consts, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 4. Escribir cada constante del pool
    for (uint32_t i = 0; i < num_consts; i++) {
        uint8_t ctype = (uint8_t)buffer.constants[i].type;
        blob[offset++] = ctype;

        uint16_t slen = (uint16_t)strlen(buffer.constants[i].as.sval);
        memcpy(blob + offset, &slen, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        memcpy(blob + offset, buffer.constants[i].as.sval, slen);
        offset += slen;
    }

    // 5. Escribir el tamaño del bytecode de instrucciones
    uint32_t code_size = (uint32_t)buffer.count;
    memcpy(blob + offset, &code_size, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 6. Escribir las instrucciones puras al final del blob
    memcpy(blob + offset, buffer.code, buffer.count);
    offset += buffer.count;

    *out_size = total_size;

    // (Opcional) Liberar la memoria temporal del buffer de compilación si es necesario
    // f_codegen_buffer_free(&buffer);

    return blob;
}

// Función interna adaptada con prefijo f_* usando el búfer
size_t f_codegen_add_chararray_const_to_buffer(BytecodeBuffer* buffer, const char* raw_str) {
    char clean_buf[256];
    const char* str_to_use = raw_str;
    
    // Limpiar comillas si el string viene delimitado desde el AST (ej. "\"sys/out\"")
    size_t len = strlen(raw_str);
    if (len >= 2 && raw_str[0] == '"' && raw_str[len - 1] == '"') {
        size_t clean_len = len - 2;
        if (clean_len < sizeof(clean_buf)) {
            memcpy(clean_buf, raw_str + 1, clean_len);
            clean_buf[clean_len] = '\0';
            str_to_use = clean_buf;
        }
    }

    // Redimensionar el pool de constantes del buffer
    if (buffer->constants_count >= buffer->constants_capacity) {
        buffer->constants_capacity = buffer->constants_capacity ? buffer->constants_capacity * 2 : 8;
        FoxyConstant* temp = realloc(buffer->constants, sizeof(FoxyConstant) * buffer->constants_capacity);
        if (temp) {
            buffer->constants = temp;
        }
    }

    size_t index = buffer->constants_count++;
    buffer->constants[index].type = FOXY_CONSTANT_CHARARRAY;
    buffer->constants[index].as.sval = strdup(str_to_use);

    return index;
}

uint8_t f_codegen_add_constant(BytecodeBuffer* buffer, FOXY_CONSTANT_TYPE type, const char* sval) {
    if (buffer->constants_count >= buffer->constants_capacity) {
        buffer->constants_capacity = buffer->constants_capacity ? buffer->constants_capacity * 2 : 4;
        FoxyConstant* temp = realloc(buffer->constants, sizeof(FoxyConstant) * buffer->constants_capacity);
        if (!temp) {
            buffer->has_error = 1;
            return 0;
        }
        buffer->constants = temp;
    }
    
    uint8_t index = (uint8_t)buffer->constants_count;
    buffer->constants[index].type = type;
    buffer->constants[index].as.sval = strdup(sval);
    buffer->constants_count++;
    
    return index;
}

// Función para compilar la carga de librerías usando el búfer real
void f_codegen_compile_load_lib(BytecodeBuffer* buffer, const char* raw_path_from_ast) {
    // 1. Limpiar comillas si las trae la ruta del include
    char clean_path[256];
    size_t len = strlen(raw_path_from_ast);
    if (len >= 2 && raw_path_from_ast[0] == '"' && raw_path_from_ast[len - 1] == '"') {
        memcpy(clean_path, raw_path_from_ast + 1, len - 2);
        clean_path[len - 2] = '\0';
    } else {
        strncpy(clean_path, raw_path_from_ast, sizeof(clean_path));
        clean_path[sizeof(clean_path) - 1] = '\0';
    }

    // 2. Registrar la ruta de la librería en el Constant Pool temporal del buffer
    uint8_t const_index = f_codegen_add_constant(buffer, FOXY_CONSTANT_CHARARRAY, clean_path);

    // 3. Emitir el opcode de carga de librería y el índice de 1 byte (¡Cero caracteres inline!)
    f_codegen_buffer_write(buffer, FOXCODE_LOAD_LIB);
    f_codegen_buffer_write(buffer, const_index);
}

// Función recursiva interna para compilar nodos del AST de forma estricta
static void compile_node(ASTNode* node, BytecodeBuffer* buffer) {
    if (!node || buffer->has_error) return;

    switch (node->type) {
        case AST_PROGRAM:
            for (uint32_t i = 0; i < node->child_count; i++) {
                compile_node(node->children[i], buffer);
            }
            break;

        case AST_INCLUDE: {
            if (node->child_count > 0 && node->children[0]) {
                ASTNode* path_node = node->children[0];
                // Pasamos el token start y length o la cadena completa para que la limpie
                // (puedes ajustar según cómo prefieras pasar el texto crudo del nodo)
                char raw_path[256];
                uint32_t val_len = path_node->token.length;
                if (val_len < sizeof(raw_path)) {
                    memcpy(raw_path, path_node->token.start, val_len);
                    raw_path[val_len] = '\0';
                    
                    // Llamamos a la función de compilación con el búfer
                    f_codegen_compile_load_lib(buffer, raw_path);
                }
            }
            break;
        }

        case AST_EXPR_CALL:
            // 1. Primero compilar y cargar los argumentos hijos en la pila
            for (uint32_t i = 0; i < node->child_count; i++) {
                compile_node(node->children[i], buffer);
            }
            // 2. Emitir la instrucción real de llamada una sola vez
            f_codegen_buffer_write(buffer, FOXCODE_CALL);
            break;

        case AST_LITERAL: {
            // 1. Duplicar los bytes exactos del token desde la fuente
            char* raw_val = strndup(node->token.start, node->token.length);
            if (!raw_val) {
                buffer->has_error = 1;
                break;
            }

            // 2. Limpiar las comillas envolventes si el literal es un string de texto
            char* clean_val = raw_val;
            size_t len = node->token.length;
            if (len >= 2 && raw_val[0] == '"' && raw_val[len - 1] == '"') {
                raw_val[len - 1] = '\0';
                clean_val = raw_val + 1;
            }

            // 3. Registrar la cadena en el Constant Pool temporal del búfer
            uint8_t const_index = f_codegen_add_constant(buffer, FOXY_CONSTANT_CHARARRAY, clean_val);

            // 4. Liberar el duplicado temporal crudo
            free(raw_val);

            // 5. ESCRIBIR EN EL CÓDIGO ÚNICAMENTE EL OPCODE Y EL ÍNDICE (¡Cero caracteres inline!)
            f_codegen_buffer_write(buffer, FOXCODE_LOAD_CONST);
            f_codegen_buffer_write(buffer, const_index);
            break;
        }

        default:
            // Otros nodos se ignoran de forma segura en esta fase base
            break;
    }
}