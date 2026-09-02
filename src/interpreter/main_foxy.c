#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "f_lexer.h"
#include "f_parser.h"
#include "f_codegen.h"
#include "f_vm.h"
#include "f_utils.h"

void f_debug_dump_bytecode(const uint8_t* blob, size_t size, const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) {
        perror("No se pudo abrir el archivo de dump");
        return;
    }

    fprintf(f, "=== Foxy Bytecode Hex Dump (Tamaño: %zu bytes) ===\n", size);
    for (size_t i = 0; i < size; i++) {
        fprintf(f, "%02X ", blob[i]);
        if ((i + 1) % 16 == 0) {
            fprintf(f, "\n");
        }
    }
    fprintf(f, "\n===============================================\n");
    fclose(f);
    printf("[Foxy Debug] Bytecode volcado a %s\n", filepath);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.foxy>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    printf("=== [Foxy Intérprete] Cargando archivo: %s ===\n", filename);

    // 1. Leer el código fuente usando f_utils_read_file
    char* source_code = f_utils_read_file(filename);
    if (!source_code) {
        fprintf(stderr, "[Foxy Error] No se pudo abrir o leer el archivo fuente: '%s'\n", filename);
        return 1;
    }

    // 2. Inicializar el Lexer
    FoxyLexer lexer;
    f_lexer_init(&lexer, source_code, filename);

    // 3. Ejecutar el Parser para obtener el AST
    ASTNode* ast_root = parser_parse(&lexer);
    printf("[Foxy Parser] AST generado con éxito. Nodos detectados en raíz: %u\n", ast_root->child_count);

    // 4. Generar bytecode real a partir del AST usando f_codegen
    size_t bytecode_size = 0;
    uint8_t* generated_bytecode = f_generate_bytecode(ast_root, &bytecode_size);
    if (!generated_bytecode) {
        fprintf(stderr, "[Foxy Codegen Error] Fallo al generar bytecode desde el AST.\n");
        free_ast(ast_root);
        free(source_code);
        return 1;
    }
    printf("[Foxy Codegen] Bytecode generado correctamente. Tamaño: %zu bytes\n", bytecode_size);
    f_debug_dump_bytecode(generated_bytecode, bytecode_size, "bytecode_dump.txt");

    // 5. Inicializar la Máquina Virtual
    FoxyVM* vm = f_vm_create();
    if (!vm) {
        fprintf(stderr, "[Foxy VM Error] No se pudo inicializar la máquina virtual.\n");
        free(generated_bytecode);
        free_ast(ast_root);
        free(source_code);
        return 1;
    }

    // 6. Cargar el script usando f_vm_load_script_to_process (Deserializa constantes y extrae el bytecode puro)
    if (!f_vm_load_script_to_process(vm, ast_root, 1)) {
        fprintf(stderr, "[Foxy VM Error] No se pudo cargar y deserializar el script en la VM.\n");
        f_vm_free(vm);
        free(generated_bytecode);
        free_ast(ast_root);
        free(source_code);
        return 1;
    }

    printf("[Foxy VM] Ejecutando ciclo de despacho para el script...\n");

    // 7. Ejecutar la VM
    int exit_code = f_vm_run(vm);
    printf("[Foxy VM] Ejecución finalizada. Código de salida: %d\n", exit_code);

    // 8. Liberación limpia de recursos (VM, bytecode dinámico, AST y código fuente)
    f_vm_free(vm);
    free(generated_bytecode);
    free_ast(ast_root);
    free(source_code);

    printf("=== [Foxy Intérprete] Proceso concluido limpiamente ===\n");
    return 0;
}