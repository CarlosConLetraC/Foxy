#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_vm.h"
#include "f_value.h"
#include "f_parser.h"
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
            if (buffer->constants[i].type == FOXY_VAL_ARRAY) {
                if (buffer->constants[i].as.array) {
                    // Si as.array apunta a un char* crudo asignado con strdup en codegen:
                    free((void*)buffer->constants[i].as.array);
                    buffer->constants[i].as.array = NULL;
                }
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
    if (!ast_root) return NULL;
    
    // 1. Inicializar buffer y compilar el AST
    BytecodeBuffer buffer;
    f_codegen_buffer_init(&buffer);
    compile_node(ast_root, &buffer);

    // ... recorrido del AST (f_codegen_visit) para llenar buffer.code y buffer.constants ...

    // 2. PASO A: Calcular el tamaño total del blob binario
    size_t constants_payload_size = 0;

    for (size_t i = 0; i < buffer.constants_count; i++) {
        switch (buffer.constants[i].type) {
            case FOXY_VAL_ARRAY: {
                const char *str = (const char*)buffer.constants[i].as.array;
                if (str) {
                    // 1 byte (Tag tipo) + 2 bytes (Longitud uint16_t) + N bytes (String)
                    constants_payload_size += 1 + sizeof(uint16_t) + strlen(str);
                }
                break;
            }
            case FOXY_VAL_INT:
                constants_payload_size += 1 + sizeof(int32_t);
                break;
            case FOXY_VAL_NUMBER:
            case FOXY_VAL_DOUBLE:
                constants_payload_size += 1 + sizeof(double);
                break;
            case FOXY_VAL_BOOL:
            case FOXY_VAL_CHAR:
                constants_payload_size += 1 + sizeof(uint8_t);
                break;
            default:
                break;
        }
    }

    // Cabecera: Cantidad de constantes (uint16_t) + Código (uint32_t) + Payloads
    size_t total_size = sizeof(uint16_t) + constants_payload_size +
                        sizeof(uint32_t) + buffer.count;

    uint8_t* blob = (uint8_t*)malloc(total_size);
    if (!blob) {
        f_codegen_buffer_free(&buffer);
        return NULL;
    }

    size_t offset = 0;

    // 3. PASO B: Escribir la cantidad de constantes
    uint16_t const_count = (uint16_t)buffer.constants_count;
    memcpy(blob + offset, &const_count, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // 4. PASO C: Serializar las constantes (Emisión del payload)
    for (size_t i = 0; i < buffer.constants_count; i++) {
        switch (buffer.constants[i].type) {
            case FOXY_VAL_ARRAY: {
                const char *str = (const char*)buffer.constants[i].as.array;
                uint16_t slen = str ? (uint16_t)strlen(str) : 0;
                
                blob[offset++] = (uint8_t)FOXY_VAL_ARRAY;
                memcpy(blob + offset, &slen, sizeof(uint16_t));
                offset += sizeof(uint16_t);
                
                if (slen > 0) {
                    memcpy(blob + offset, str, slen);
                    offset += slen;
                }
                break;
            }
            case FOXY_VAL_INT:
                blob[offset++] = (uint8_t)FOXY_VAL_INT;
                memcpy(blob + offset, &buffer.constants[i].as.ival, sizeof(int32_t));
                offset += sizeof(int32_t);
                break;
            case FOXY_VAL_NUMBER:
            case FOXY_VAL_DOUBLE:
                blob[offset++] = (uint8_t)buffer.constants[i].type;
                memcpy(blob + offset, &buffer.constants[i].as.dval, sizeof(double));
                offset += sizeof(double);
                break;
            case FOXY_VAL_BOOL:
                blob[offset++] = (uint8_t)FOXY_VAL_BOOL;
                blob[offset++] = buffer.constants[i].as.bval ? 1 : 0;
                break;
            default:
                break;
        }
    }

    // 5. PASO D: Escribir el bloque de instrucciones (instrucciones de la VM)
    uint32_t code_count = (uint32_t)buffer.count;
    memcpy(blob + offset, &code_count, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer.count > 0) {
        memcpy(blob + offset, buffer.code, buffer.count);
        offset += buffer.count;
    }

    if (out_size) *out_size = total_size;

    f_codegen_buffer_free(&buffer);
    return blob;
}

// Función interna adaptada con prefijo f_* usando el búfer
size_t f_codegen_add_chararray_const_to_buffer(BytecodeBuffer* buffer, const char* raw_str) {
    if (!buffer || !raw_str) return (size_t)-1;

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

    // Redimensionar el pool de constantes del buffer usando FoxyValue
    if (buffer->constants_count >= buffer->constants_capacity) {
        size_t new_cap = buffer->constants_capacity ? buffer->constants_capacity * 2 : 8;
        FoxyValue* temp = (FoxyValue*)realloc(buffer->constants, sizeof(FoxyValue) * new_cap);
        if (!temp) {
            return (size_t)-1; // Fallo en la asignación de memoria
        }
        buffer->constants = temp;
        buffer->constants_capacity = new_cap;
    }

    size_t index = buffer->constants_count++;
    buffer->constants[index].type = FOXY_VAL_ARRAY;
    buffer->constants[index].as.array = (FoxyArray*)strdup(str_to_use);

    return index;
}

uint8_t f_codegen_add_constant(BytecodeBuffer* buffer, FoxyValueType type, const char* sval) {
    uint8_t index = buffer->constants_count++;
    buffer->constants[index].type = type;
    
    // Guardamos la cadena duplicada en el miembro array del union (o alloc de FoxyArray)
    buffer->constants[index].as.array = (FoxyArray*)strdup(sval);
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
    uint8_t const_index = f_codegen_add_constant(buffer, FOXY_VAL_ARRAY, clean_path);

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
            uint8_t const_index = f_codegen_add_constant(buffer, FOXY_VAL_ARRAY, clean_val);

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

// Protoboard para llamadas recursivas dentro del módulo
void f_codegen_emit_byte(FoxyCodegenContext *ctx, uint8_t byte);
uint32_t f_codegen_add_constant_string(FoxyCodegenContext *ctx, const char *str);
uint8_t f_codegen_get_or_add_local(FoxyCodegenContext *ctx, const char *name);
void f_codegen_visit(FoxyCodegenContext *ctx, ASTNode *node);
static char* token_to_string(FoxyToken token);

// ==========================================
// Funciones Visitadoras por Tipo de Nodo AST
// ==========================================

void f_codegen_free(FoxyCodegenContext *ctx) {
    if (!ctx) return;

    // 1. Liberar el buffer de bytecode
    if (ctx->code) {
        free(ctx->code);
        ctx->code = NULL;
    }

    // 2. Liberar el Constant Pool y sus elementos dinámicos
    if (ctx->constants) {
        for (size_t i = 0; i < ctx->constants_count; i++) {
            if (ctx->constants[i].type == FOXY_VAL_ARRAY) {
                if (ctx->constants[i].as.array) {
                    free((void*)ctx->constants[i].as.array);
                    ctx->constants[i].as.array = NULL;
                }
            }
        }
        free(ctx->constants);
        ctx->constants = NULL;
    }

    // 3. Liberar la tabla de símbolos de variables locales
    if (ctx->locals) {
        free(ctx->locals);
        ctx->locals = NULL;
    }

    // 4. Liberar la estructura principal del contexto
    free(ctx);
}

FoxyCodegenContext* f_codegen_create(void) {
    FoxyCodegenContext *ctx = (FoxyCodegenContext*)malloc(sizeof(FoxyCodegenContext));
    if (!ctx) return NULL;

    // Bytecode buffer
    ctx->code_capacity = 256;
    ctx->code_size = 0;
    ctx->code = (uint8_t*)malloc(sizeof(uint8_t) * ctx->code_capacity);

    // Constant pool
    ctx->constants_capacity = 16;
    ctx->constants_count = 0;
    ctx->constants = (FoxyConstant*)malloc(sizeof(FoxyConstant) * ctx->constants_capacity);

    // Tabla de locales
    ctx->locals_capacity = 16;
    ctx->locals_count = 0;
    ctx->locals = (FoxyLocalSymbol*)malloc(sizeof(FoxyLocalSymbol) * ctx->locals_capacity);

    if (!ctx->code || !ctx->constants || !ctx->locals) {
        f_codegen_free(ctx);
        return NULL;
    }

    return ctx;
}

void f_codegen_steal_constants(FoxyCodegenContext *ctx, FoxyConstant **out_constants, size_t *out_count) {
    if (!ctx) return;

    if (out_constants) *out_constants = ctx->constants;
    if (out_count) *out_count = ctx->constants_count;

    // Desvincular del contexto
    ctx->constants = NULL;
    ctx->constants_count = 0;
    ctx->constants_capacity = 0;
}

void f_codegen_steal_resources(FoxyCodegenContext *ctx, uint8_t **out_code, size_t *out_code_size, FoxyConstant **out_constants, size_t *out_constants_count) {
    if (!ctx) return;

    if (out_code) *out_code = ctx->code;
    if (out_code_size) *out_code_size = ctx->code_size;
    ctx->code = NULL;
    ctx->code_size = 0;
    ctx->code_capacity = 0;

    if (out_constants) *out_constants = ctx->constants;
    if (out_constants_count) *out_constants_count = ctx->constants_count;
    ctx->constants = NULL;
    ctx->constants_count = 0;
    ctx->constants_capacity = 0;
}

void f_codegen_emit_byte(FoxyCodegenContext *ctx, uint8_t byte) {
    if (!ctx) return;
    if (ctx->code_size >= ctx->code_capacity) {
        ctx->code_capacity *= 2;
        ctx->code = (uint8_t*)realloc(ctx->code, ctx->code_capacity);
    }
    ctx->code[ctx->code_size++] = byte;
}

static char* token_to_string(FoxyToken token) {
    if (!token.start || token.length == 0)
        return strdup("");
    char* buf = (char*)malloc(token.length + 1);
    if (!buf) return NULL;
    memcpy(buf, token.start, token.length);
    buf[token.length] = '\0';
    return buf;
}

uint8_t f_codegen_get_or_add_local(FoxyCodegenContext *ctx, const char *name) {
    if (!ctx || !name) return 0;

    // 1. Buscar si la variable local ya fue declarada previamente
    for (size_t i = 0; i < ctx->locals_count; i++) {
        if (strcmp(ctx->locals[i].name, name) == 0)
            return ctx->locals[i].index;
    }

    // 2. Si no existe, verificar capacidad del arreglo de locales
    if (ctx->locals_count >= ctx->locals_capacity) {
        ctx->locals_capacity = ctx->locals_capacity == 0 ? 16 : ctx->locals_capacity * 2;
        FoxyLocalSymbol *temp = (FoxyLocalSymbol*)realloc(ctx->locals, sizeof(FoxyLocalSymbol) * ctx->locals_capacity);
        if (!temp) {
            fprintf(stderr, "[Foxy Codegen Error] Fallo al redimensionar la tabla de locales.\n");
            return 0;
        }
        ctx->locals = temp;
    }

    // 3. Registrar la nueva variable local
    uint8_t index = (uint8_t)ctx->locals_count;
    strncpy(ctx->locals[index].name, name, sizeof(ctx->locals[index].name) - 1);
    ctx->locals[index].name[sizeof(ctx->locals[index].name) - 1] = '\0';
    ctx->locals[index].index = index;

    ctx->locals_count++;

    return index;
}

// AST_PROGRAM: Nodo raíz del script
static void f_codegen_visit_program(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;
    for (uint32_t i = 0; i < node->child_count; i++)
        f_codegen_visit(ctx, node->children[i]);
}

// AST_INCLUDE
static void f_codegen_visit_include(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    if (node->child_count > 0 && node->children[0]) {
        ASTNode* path_node = node->children[0];
        char *raw_path = token_to_string(path_node->token);

        // Limpiar comillas si el literal las conserva
        char *clean_path = raw_path;
        size_t len = strlen(raw_path);
        if (len >= 2 && raw_path[0] == '"' && raw_path[len - 1] == '"') {
            raw_path[len - 1] = '\0';
            clean_path = raw_path + 1;
        }

        uint32_t const_idx = f_codegen_add_constant_string(ctx, clean_path);
        free(raw_path);

        f_codegen_emit_byte(ctx, FOXCODE_LOAD_LIB);
        f_codegen_emit_byte(ctx, (uint8_t)const_idx);
    }
}

// AST_LITERAL
static void f_codegen_visit_literal(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    char *raw_val = token_to_string(node->token);
    char *clean_val = raw_val;
    size_t len = strlen(raw_val);

    // Remover comillas si es un literal de cadena (String)
    if (len >= 2 && raw_val[0] == '"' && raw_val[len - 1] == '"') {
        raw_val[len - 1] = '\0';
        clean_val = raw_val + 1;
    }

    uint32_t const_idx = f_codegen_add_constant_string(ctx, clean_val);
    free(raw_val);

    f_codegen_emit_byte(ctx, FOXCODE_LOAD_CONST);
    f_codegen_emit_byte(ctx, (uint8_t)const_idx);
}

// AST_EXPR_CALL
static void f_codegen_visit_expr_call(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    // 1. Apilar los argumentos del método/función
    for (uint32_t i = 0; i < node->child_count; i++) {
        f_codegen_visit(ctx, node->children[i]);
    }

    // 2. Apilar el identificador o la referencia ejecutable
    char *func_name = token_to_string(node->token);
    char *clean_name = func_name;
    size_t len = strlen(func_name);
    
    if (len >= 2 && func_name[0] == '"' && func_name[len - 1] == '"') {
        func_name[len - 1] = '\0';
        clean_name = func_name + 1;
    }

    uint32_t const_idx = f_codegen_add_constant_string(ctx, clean_name);
    free(func_name);

    f_codegen_emit_byte(ctx, FOXCODE_LOAD_CONST);
    f_codegen_emit_byte(ctx, (uint8_t)const_idx);

    // 3. Emitir el opcode de llamada universal
    f_codegen_emit_byte(ctx, FOXCODE_CALL);
}

// AST_VAR_DECL
static void f_codegen_visit_var_decl(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    if (node->child_count > 0)
        f_codegen_visit(ctx, node->children[0]);
    else
        f_codegen_emit_byte(ctx, FOXCODE_LOAD_NULL);

    char *var_name = token_to_string(node->token);
    uint8_t local_idx = f_codegen_get_or_add_local(ctx, var_name);
    free(var_name);

    f_codegen_emit_byte(ctx, FOXCODE_STORE_LOCAL);
    f_codegen_emit_byte(ctx, local_idx);
}

// AST_ASSIGNMENT
static void f_codegen_visit_assignment(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    if (node->child_count > 0)
        f_codegen_visit(ctx, node->children[0]);

    char *var_name = token_to_string(node->token);
    uint8_t local_idx = f_codegen_get_or_add_local(ctx, var_name);
    free(var_name);

    f_codegen_emit_byte(ctx, FOXCODE_STORE_LOCAL);
    f_codegen_emit_byte(ctx, local_idx);
}

// AST_FUNCTION_DEF: Declaración de funciones Foxy
static void f_codegen_visit_function_def(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;
    // Emisión de encabezado de función/jump sobre el cuerpo según tu especificación
    for (uint32_t i = 0; i < node->child_count; i++)
        f_codegen_visit(ctx, node->children[i]);
}

// AST_CLASS_DEF: Declaración de clases u objetos
static void f_codegen_visit_class_def(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;
    for (uint32_t i = 0; i < node->child_count; i++)
        f_codegen_visit(ctx, node->children[i]);
}

// AST_IF_STMT: Estructuras condicionales
static void f_codegen_visit_if_stmt(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;
    // Visitar condición y bloques then/else
    for (uint32_t i = 0; i < node->child_count; i++)
        f_codegen_visit(ctx, node->children[i]);
}

// AST_FOR_STMT: Bucles for
static void f_codegen_visit_for_stmt(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;
    for (uint32_t i = 0; i < node->child_count; i++)
        f_codegen_visit(ctx, node->children[i]);
}

// AST_RETURN_STMT: Retorno de valores
static void f_codegen_visit_return_stmt(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    if (node->child_count > 0)
        f_codegen_visit(ctx, node->children[0]);
    else
        f_codegen_emit_byte(ctx, FOXCODE_LOAD_NULL);
    // Si manejas FOXCODE_RETURN en tu enum, emítelo aquí
}

// ==========================================
// Despachador Principal del Codegen
// ==========================================

void f_codegen_visit(FoxyCodegenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return;

    switch (node->type) {
        case AST_PROGRAM:
            f_codegen_visit_program(ctx, node);
            break;
        case AST_INCLUDE:
            f_codegen_visit_include(ctx, node);
            break;
        case AST_LITERAL:
            f_codegen_visit_literal(ctx, node);
            break;
        case AST_EXPR_CALL:
            f_codegen_visit_expr_call(ctx, node);
            break;
        case AST_VAR_DECL:
            f_codegen_visit_var_decl(ctx, node);
            break;
        case AST_ASSIGNMENT:
            f_codegen_visit_assignment(ctx, node);
            break;
        case AST_FUNCTION_DEF:
            f_codegen_visit_function_def(ctx, node);
            break;
        case AST_CLASS_DEF:
            f_codegen_visit_class_def(ctx, node);
            break;
        case AST_IF_STMT:
            f_codegen_visit_if_stmt(ctx, node);
            break;
        case AST_FOR_STMT:
            f_codegen_visit_for_stmt(ctx, node);
            break;
        case AST_RETURN_STMT:
            f_codegen_visit_return_stmt(ctx, node);
            break;
        default:
            fprintf(stderr, "[Foxy Codegen Warning] Tipo de nodo AST no soportado: %d\n", node->type);
            break;
    }
}

uint32_t f_codegen_add_constant_string(FoxyCodegenContext* ctx, const char* str) {
    for (size_t i = 0; i < ctx->constants_count; i++) {
        if (ctx->constants[i].type == FOXY_VAL_ARRAY && ctx->constants[i].as.array) {
            const char* current_str = (const char*)ctx->constants[i].as.array;
            if (strcmp(current_str, str) == 0)
                return (uint32_t)i;
        }
    }

    uint32_t idx = (uint32_t)ctx->constants_count++;
    ctx->constants[idx].type = FOXY_VAL_ARRAY;
    ctx->constants[idx].as.array = (FoxyArray*)strdup(str);
    return idx;
}