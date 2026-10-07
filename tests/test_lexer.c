#include "f_lexer.h"
#include <stdio.h>

int main(int argc, char **argv) {
    const char *filepath = (argc > 1) ? argv[1] : "examples/CasoLoco.foxy";

    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", filepath);
        return 1;
    }

    FoxyLexer lexer;
    if (!f_lexer_init_file(&lexer, file, filepath)) {
        fprintf(stderr, "Aviso: El archivo '%s' está vacío o ocurrió un error al inicializar el lexer.\n", filepath);
        /* f_lexer_init_file libera y cierra el FILE* automáticamente si falla/está vacío */
        return 0;
    }

    printf("=== Tokenizing: %s ===\n", filepath);
    FoxyToken token;
    do {
        token = f_lexer_next_token(&lexer);
        f_lexer_print_token(&token);
    } while (token.type != FOX_TOKEN_EOF && token.type != FOX_TOKEN_ERROR);

    f_lexer_close(&lexer);
    return 0;
}