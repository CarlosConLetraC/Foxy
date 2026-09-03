#!/usr/bin/env bash

# Directorio base para buscar (puedes cambiarlo si deseas buscar en src/ también)
HEADERS_DIR="include"

# Si el primer argumento es un directorio existente, lo usa como ruta
if [ -d "$1" ]; then
    HEADERS_DIR="$1"
    shift
fi

echo "=================================================="
echo " Buscando duplicados en: ${HEADERS_DIR}/"
echo "=================================================="

if [ ! -d "$HEADERS_DIR" ]; then
    echo "Error: El directorio '$HEADERS_DIR' no existe."
    exit 1
fi

# Si se pasaron argumentos adicionales, busca cada término recibido
if [ "$#" -gt 0 ]; then
    for term in "$@"; do
        echo -e "\n\033[1;32m--> Coincidencias para '$term':\033[0m"
        grep -rn --color=always "$term" "$HEADERS_DIR" 2>/dev/null | \
        while IFS=: read -r file line content; do
            printf "\033[1;34m%-35s\033[0m \033[1;33mLínea %-4s\033[0m : %s\n" "$file" "$line" "$content"
        done
    done
else
    # Comportamiento por defecto si no se pasan argumentos:
    # Busca todas las definiciones de typedef struct, typedef enum y enum
    echo -e "\033[1;32m--> Buscando todas las declaraciones de structs y enums...\033[0m\n"
    grep -rnE 'typedef[[:space:]]+(struct|enum)([[:space:]]+[A-Za-z0-9_]+)?|^enum[[:space:]]+[A-Za-z0-9_]+' "$HEADERS_DIR" 2>/dev/null | \
    while IFS=: read -r file line content; do
        printf "\033[1;34m%-35s\033[0m \033[1;33mLínea %-4s\033[0m : %s\n" "$file" "$line" "$content"
    done
fi

echo -e "\n=================================================="