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
        f_utils_dump_constant_pool(cg.constants, cg.constants_count);
        f_utils_dump_bytecode(cg.bytecode, cg.code_count);
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
    f_ast_node_free((FoxyASTNode*)ast_root);
    free(source);

    return exit_code;
}