gcc -O3 -Wall testlexer.c ../src/core/f_lexer.c -I../include -o foxy_lexer && for archivo in ../examples/*.foxy; do ./foxy_lexer "$archivo" >> resultados_lexer.txt; done
