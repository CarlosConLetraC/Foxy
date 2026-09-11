#include <stdio.h>
#include <stdlib.h>
#include "f_lexer.h"

static char *read_file(const char *path);

int main(int argc, char **argv) {
    const char *filepath = (argc > 1) ? argv[1] : "foo.foxy";

    char *source = read_file(filepath);
    if (!source) return 1;

    FoxyLexer lexer;
    f_lexer_init(&lexer, source, filepath);

    printf("=== ANALIZANDO: %s ===\n", filepath);

    FoxyToken token;
    do {
        token = f_lexer_next_token(&lexer);
        
        printf("[%s:%05u:%05u] Cat: %03u | Tipo: %-3u | Lexema: %.*s\n",
               token.pos.filename ? token.pos.filename : filepath,
               token.pos.line,
               token.pos.column,
               token.type_category,
               token.type,
               (int)token.length,
               token.start);

    } while (token.type != FOX_TOKEN_EOF && token.type != FOX_TOKEN_ERROR);

    free(source);
    return 0;
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memoria insuficiente para leer '%s'\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}