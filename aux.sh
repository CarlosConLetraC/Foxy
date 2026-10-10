alias fecha_creacion_repo="curl -s https://api.github.com/repos/CarlosConLetraC/Foxy | jq '.created_at'"

function doall() {
    local arg="examples/HeavyTest.foxy"
    local bin_ast="./build/tests/ast"
    local bin_lexer="./build/tests/lexer"

    make clean
    
    make ast_test && {
        stdbuf -oL -eL "$bin_ast" "$arg" &> resultado_ast.txt
        echo "Código de salida AST: $?"
    } || echo "Error en make ast_test"
    
    echo ""
    
    make lexer_test && {
        stdbuf -oL -eL "$bin_lexer" "$arg" &> resultado_lexer.txt
        echo "Código de salida Lexer: $?"
    } || echo "Error en make lexer_test"
    
    echo ""
}