#include "f_ast.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "[Error Test AST] Se esperaba un argumento en sesión CLI.\n");
        return EXIT_FAILURE;
    }
    const char *filepath = argv[1];

    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "[Error Test AST] No se pudo abrir el archivo: %s\n", filepath);
        return EXIT_FAILURE;
    }

    printf("=== PARSEANDO ARCHIVO FOXY: %s ===\n\n", filepath);

    FoxyAstParser parser;
    f_ast_parser_init(&parser, file, filepath);

    /* Construye el AST directamente desde el stream del archivo */
    FoxyAstNode *program_ast = f_ast_parse_program(&parser);

    fclose(file);

    if (parser.had_error || !program_ast) {
        fprintf(stderr, "[Error Test AST] Ocurrieron errores durante el parsing.\n");
        if (program_ast) f_ast_free_node(program_ast);
        return EXIT_FAILURE;
    }

    /* Imprime la estructura jerárquica del AST en consola */
    printf("--- ESTRUCTURA DEL AST GENERADO ---\n");
    f_ast_print(program_ast, 0);

    /* Libera recursivamente toda la memoria asignada al AST */
    f_ast_free_node(program_ast);

    printf("\n=== PRUEBA DE AST CON %s FINALIZADA CON ÉXITO ===\n", filepath);
    return EXIT_SUCCESS;
}