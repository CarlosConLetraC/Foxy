#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "f_lexer.h"
#include "f_parser.h"
#include "f_codegen.h"
#include "f_ast.h"
#include "f_vm.h"
#include "f_utils.h"

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

    // 1. Carga de archivo utilizando f_utils_read_file
    char* source = f_utils_read_file(filename);
    if (!source) return 1;

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

    if (!f_codegen_generate(&cg, ast_root)) {
        fprintf(stderr, "[Error] Fallo la generación de bytecode.\n");
        f_codegen_free(&cg);
        f_ast_node_free(ast_root);
        free(source);
        return 1;
    }

    // Banderas de depuración (-d / --debug-bytecode)
    if (debug_mode) {
        f_utils_dump_constant_pool(cg.constants, cg.constants_count);
        f_utils_dump_bytecode(cg.bytecode, cg.code_count);
    }

    FoxyVM *vm = f_vm_new();
    if (!vm) {
        f_codegen_free(&cg);
        f_ast_node_free(ast_root);
        free(source);
        return 1;
    }

    // Transferir la propiedad del pool de constantes a la VM
    vm->constants = cg.constants;
    vm->constants_count = cg.constants_count;
    vm->constants_capacity = cg.constants_capacity;

    // Desvincular de cg para que f_codegen_free no libere el pool de constantes asignado a la VM
    cg.constants = NULL;
    cg.constants_count = 0;
    cg.constants_capacity = 0;

    // Cargar el proceso
    f_vm_load_process(vm, (const uint8_t*)cg.bytecode, cg.code_count * sizeof(FoxInstruction), filename);

    // Liberar recursos de compilación
    f_codegen_free(&cg); // Solo liberará cg.bytecode limpiamente
    f_ast_node_free(ast_root);
    free(source);

    // Ejecución de la VM
    FoxyStatus status = f_vm_run(vm);
    int exit_code = (status == FOXY_STATUS_SUCCESS) ? 0 : 1;

    // Liberar la VM completamente
    f_vm_free(vm);

    return exit_code;
}