#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "f_lexer.h"
#include "f_parser.h"
#include "f_codegen.h"
#include "f_ast.h"
#include "f_vm.h"

static void print_usage(const char* prog_name) {
    printf("Uso: %s [opciones] <archivo.foxy>\n", prog_name);
    printf("Opciones:\n");
    printf("  -d, --debug-bytecode  Imprime el bytecode y constantes generadas antes de ejecutar\n");
    printf("  -h, --help            Muestra este mensaje de ayuda\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    bool debug_mode = false;
    const char* filename = NULL;

    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug-bytecode") == 0) {
            debug_mode = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "[Error] Opción desconocida: %s\n", argv[i]);
            return 1;
        }
    }

    if (!filename) {
        fprintf(stderr, "[Error] No se especificó archivo de entrada.\n");
        return 1;
    }

    // 1. Carga de archivo
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "[Error] No se pudo abrir el archivo: %s\n", filename);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = (char*)malloc(length + 1);
    if (!source) {
        fprintf(stderr, "[Error] Memoria insuficiente para leer el archivo.\n");
        fclose(file);
        return 1;
    }
    fread(source, 1, length, file);
    fclose(file);
    source[length] = '\0';

    // 2. Lexer y Parser
    FoxyLexer lexer;
    f_lexer_init(&lexer, source, filename);
    FoxyASTNode* ast_root = f_parser_parse(&lexer);

    if (!ast_root) {
        fprintf(stderr, "[Error] Fallo al generar el AST.\n");
        free(source);
        return 1;
    }

    // 3. Generación de Código (Codegen)
    FoxyCodegen cg;
    f_codegen_init(&cg);

    // Hacer el cast a (FoxyASTNode*) si f_codegen_generate espera ese tipo
    if (!f_codegen_generate(&cg, (FoxyASTNode*)ast_root)) {
        fprintf(stderr, "[Error] Fallo durante la generación de bytecode.\n");
        f_codegen_free(&cg);
        f_ast_node_free((FoxyASTNode*)ast_root);
        free(source);
        return 1;
    }

    // Banderas de depuración (-d / --debug-bytecode)
    if (debug_mode) {
        printf("=== [DEBUG] CONSTANT POOL (%zu elementos) ===\n", cg.constants_count);
        for (size_t i = 0; i < cg.constants_count; i++) {
            switch (cg.constants[i].type) {
                case FOXY_VAL_ARRAY: {
                    FoxyArray *arr = cg.constants[i].as.array;
                    if (arr) {
                        if (arr->element_type_id == FOXY_VAL_CHAR && arr->data) {
                            printf("  [%02zu] CHAR ARRAY (String): \"%s\"\n", i, (const char*)arr->data);
                        } else {
                            printf("  [%02zu] GENERIC ARRAY (Elem Type: %d, Length: %zu)\n", 
                                i, arr->element_type_id, arr->length);
                        }
                    } else {
                        printf("  [%02zu] ARRAY: null\n", i);
                    }
                    break;
                }
                case FOXY_VAL_OBJECT: {
                    const char *obj_str = cg.constants[i].as.object ? (const char *)cg.constants[i].as.object : "null";
                    printf("  [%02zu] OBJECT/STRING: \"%s\"\n", i, obj_str);
                    break;
                }
                case FOXY_VAL_INT:
                    printf("  [%02zu] INT: %ld\n", i, cg.constants[i].as.ival);
                    break;
                case FOXY_VAL_NUMBER:
                case FOXY_VAL_DOUBLE:
                    printf("  [%02zu] DOUBLE: %f\n", i, cg.constants[i].as.dval);
                    break;
                case FOXY_VAL_BOOL:
                    printf("  [%02zu] BOOL: %s\n", i, cg.constants[i].as.boolean ? "true" : "false");
                    break;
                case FOXY_VAL_NULL:
                    printf("  [%02zu] NULL\n", i);
                    break;
                default:
                    printf("  [%02zu] UNKNOWN TYPE (%d)\n", i, cg.constants[i].type);
                    break;
            }
        }
        
        // El bytecode está compuesto por instrucciones de 32 bits (FoxInstruction)
        printf("\n=== [DEBUG] BYTECODE GENERADO (%zu instrucciones / %zu bytes) ===\n", 
            cg.code_count, cg.code_count * sizeof(FoxInstruction));
        for (size_t i = 0; i < cg.code_count; i++) {
            printf("%08X ", cg.bytecode[i]);
            if ((i + 1) % 8 == 0) printf("\n");
        }
        if (cg.code_count % 8 != 0) printf("\n");
        printf("============================================================\n\n");
    }

    // 4. Transferir recursos a la VM
    FoxyVM* vm = f_vm_new();
    
    // Cargar el búfer de bytecode de 32 bits en la VM
    f_vm_load_process(vm, (uint8_t*)cg.bytecode, cg.code_count * sizeof(FoxInstruction), filename);

    // Asignar el pool de constantes transferido a la VM
    vm->constants = cg.constants;
    vm->constants_count = cg.constants_count;
    vm->constants_capacity = cg.constants_capacity;

    // Invalidar los punteros del codegen para que f_codegen_free no los libere 
    // ya que ahora la VM asume la propiedad de los búferes.
    cg.bytecode = NULL;
    cg.constants = NULL;
    f_codegen_free(&cg);

    // 5. Ejecución y limpieza final
    int exit_code = f_vm_run(vm);

    f_vm_free(vm);
    f_ast_node_free((FoxyASTNode*)ast_root); // <-- Añadir el cast aquí
    free(source);

    return exit_code;
}