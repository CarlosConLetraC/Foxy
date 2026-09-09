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

    // 1. Process command-line flags (-d/--debug-bytecode, -h/--help)
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

    // 2. Read input file
    char* source = f_utils_read_file(filename);
    if (!source) return 1;

    // 3. Tokenize & Parse AST
    FoxyLexer lexer;
    f_lexer_init(&lexer, source, filename);

    FoxyParser parser;
    f_parser_init(&parser, &lexer);

    FoxyASTNode* ast_root = f_parser_parse(&parser, NULL);

    if (!ast_root) {
        fprintf(stderr, "[Error] Fallo al generar el AST.\n");
        free(source);
        return 1;
    }

    // 4. Bytecode Generation
    FoxyCodegen cg;
    f_codegen_init(&cg);

    if (!f_codegen_generate(&cg, ast_root, NULL)) {
        fprintf(stderr, "[Error] Fallo la generación de bytecode.\n");
        f_codegen_free(&cg, NULL);
        f_ast_node_free(ast_root, NULL);
        free(source);
        return 1;
    }

    // 5. Virtual Machine Instantiation
    FoxyVM *vm = f_vm_new();
    if (!vm) {
        f_codegen_free(&cg, NULL);
        f_ast_node_free(ast_root, NULL);
        free(source);
        return 1;
    }

    // 6. Transferir pool de constantes
    vm->constants = cg.constants;
    vm->constants_count = cg.constants_count;
    vm->constants_capacity = cg.constants_capacity;

    cg.constants = NULL;
    cg.constants_count = 0;
    cg.constants_capacity = 0;

    // Cargar el proceso en la máquina virtual pasándole el tamaño exacto del buffer
    f_vm_load_process(vm, (const uint8_t*)cg.bytecode, cg.code_count * sizeof(FoxInstruction), filename);

    // 8. Debug Dumping (Pantalla y Archivo .txt)
    if (debug_mode) {
        f_utils_dump_constant_pool(vm->constants, vm->constants_count);
        f_utils_dump_bytecode(cg.bytecode, cg.code_count);

        // Exportación directa a archivo txt previa a la liberación de cg.bytecode
        f_utils_dump_bytecode_to_file(
            cg.bytecode, 
            cg.code_count, 
            vm, 
            "bytecode_dump.txt"
        );
    }

    // 9. Free Compilation Pipeline Memory
    f_codegen_free(&cg, vm);
    f_ast_node_free(ast_root, vm);
    free(source);

    // 10. Execute VM
    FoxyStatus status = f_vm_run(vm);
    int exit_code = (status == FOXY_STATUS_SUCCESS) ? 0 : 1;

    // 11. Teardown VM
    f_vm_free(vm);

    return exit_code;
}