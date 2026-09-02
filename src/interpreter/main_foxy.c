#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "f_lexer.h"
#include "f_parser.h"
#include "f_codegen.h"
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
    fread(source, 1, length, file);
    fclose(file);
    source[length] = '\0';

    // 2. Lexer y Parser
    FoxyLexer lexer;
    f_lexer_init(&lexer, source, filename);
    ASTNode* ast_root = f_parser_parse(&lexer);

    if (!ast_root) {
        fprintf(stderr, "[Error] Fallo al generar el AST.\n");
        free(source);
        return 1;
    }

    // 3. Generación de Código (Codegen)
    FoxyCodegenContext* ctx = f_codegen_create();
    if (!ctx) {
        fprintf(stderr, "[Error] No se pudo crear el contexto de codegen.\n");
        free(source);
        return 1;
    }

    // Poblar el contexto recorriendo el AST
    f_codegen_visit(ctx, ast_root);

    // Banderas de depuración (-d / --debug-bytecode)
    if (debug_mode) {
        printf("=== [DEBUG] CONSTANT POOL (%zu elementos) ===\n", ctx->constants_count);
        for (size_t i = 0; i < ctx->constants_count; i++) {
            switch (ctx->constants[i].type) {
                case FOXY_VAL_ARRAY:
                    printf("  [%02zu] STRING: \"%s\"\n", i, (char*)ctx->constants[i].as.array);
                    break;
                case FOXY_VAL_INT:
                    printf("  [%02zu] INT: %d\n", i, ctx->constants[i].as.ival);
                    break;
                case FOXY_VAL_NUMBER:
                case FOXY_VAL_DOUBLE:
                    printf("  [%02zu] DOUBLE: %f\n", i, ctx->constants[i].as.dval);
                    break;
                case FOXY_VAL_BOOL:
                    printf("  [%02zu] BOOL: %s\n", i, ctx->constants[i].as.bval ? "true" : "false");
                    break;
                default:
                    printf("  [%02zu] UNKNOWN TYPE (%d)\n", i, ctx->constants[i].type);
                    break;
            }
        }
        printf("\n=== [DEBUG] BYTECODE GENERADO (%zu bytes) ===\n", ctx->code_size);
        for (size_t i = 0; i < ctx->code_size; i++) {
            printf("%02X ", ctx->code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n============================================\n\n");
    }

    // 4. Transferir punteros a la VM antes de liberar ctx
    uint8_t *code_ptr = NULL;
    size_t code_size = 0;
    FoxyConstant *constants_ptr = NULL;
    size_t constants_count = 0;

    f_codegen_steal_resources(ctx, &code_ptr, &code_size, &constants_ptr, &constants_count);

    FoxyVM* vm = f_vm_create();
    f_vm_load_process(vm, code_ptr, code_size, filename);
    vm->constants = constants_ptr;
    vm->constants_count = constants_count;

    // Destruir ctx sin riesgo de double free sobre code y constants
    f_codegen_free(ctx);

    // Ejecución y limpieza final
    int exit_code = f_vm_run(vm);

    f_vm_free(vm);
    free(source);

    return exit_code;
}