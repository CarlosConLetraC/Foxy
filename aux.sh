alias fecha_creacion_repo="curl -s https://api.github.com/repos/CarlosConLetraC/Foxy | jq '.created_at'"

function doall() {
    local path_pruebas="./examples"
    local pruebas=(
        "CustomTypes.foxy"
        "GeneralExample.foxy"
        "HeavyTest.foxy"
        "AST_Inteligente.foxy"
    )
    local bin_path="./build/tests"
    local bins=(
        "lexer"
        "ast"
    )

    make clean

    for bin in "${bins[@]}"; do
        echo "Iniciando prueba para: $bin_path/$bin"
        
        for prueba in "${pruebas[@]}"; do
            local arg="$path_pruebas/$prueba"
            local suffix=$(basename "$prueba" .foxy)
            local target_bin="$bin_path/$bin"

            if make "${bin}_test"; then
                if [ -x "$target_bin" ]; then
                    stdbuf -oL -eL "$target_bin" "$arg" &> "resultado_${bin}_${suffix}.txt"
                    
                    local exit_code=$?
                    echo "Código de salida ${bin^^} ($prueba): $exit_code"
                else
                    echo "Error: El binario '$target_bin' no se encontró o no tiene permisos de ejecución."
                fi
            else
                echo "Error en make ${bin}_test"
            fi
            
            echo ""
        done
    done
}