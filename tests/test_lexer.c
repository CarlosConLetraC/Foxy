#include "f_exceptions.h"
#include "f_lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "[Error Test LEXER] Se esperaba un argumento en sesión CLI.\n");
        return EXIT_FAILURE;
    }
    const char *filepath = argv[1];

    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", filepath);
        return 1;
    }

    FoxyExceptionController controller;
    f_exception_controller_init(&controller);

    FoxyLexer lexer;
    if (!f_lexer_init_file(&lexer, file, filepath, &controller)) {
        fprintf(stderr, "Aviso: El archivo '%s' está vacío o ocurrió un error al inicializar el lexer.\n", filepath);
        f_exception_controller_init(&controller);
        return 0;
    }

    printf("=== Tokenizing: %s ===\n", filepath);
    FoxyToken token;
    int exit_code = 0;

    do {
        token = f_lexer_next_token(&lexer);
        f_lexer_print_token(&token);

        /* Verificar si el token generado es de categoría de error */
        if (token.type_category == FOXY_TOKEN_CAT_ERROR) {
            /* Extraer el código de excepción empaquetado en el subtipo */
            FoxyExceptionCode ex_code = (FoxyExceptionCode)(token.type & FOXMODE_MASK_12BIT);
            
            /* Emitir el error formalmente usando la tabla LUT y pasar el controlador como 1er argumento */
            exit_code = f_exception_raise(
                &controller, 
                ex_code, 
                lexer.filename, 
                lexer.line, 
                lexer.column, 
                "Lexical error encountered at lexeme '%.*s'", 
                token.length, 
                token.start
            );
            break;
        }
    } while (token.type != f_lexer_compose_type(FOXY_TOKEN_CAT_KEYWORD, FOX_TOKEN_EOF));

    f_lexer_close(&lexer);
    return exit_code;
}