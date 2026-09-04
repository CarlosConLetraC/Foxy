#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_parser.h"
#include "f_utils.h"
#include "f_ast.h"
#include "f_lexer.h"

// Obtener una copia en cadena (null-terminated) del lexema del token
char* token_to_string(FoxyToken *token) {
    if (token->subtype == FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY) {
        // Copiar el contenido omitiendo las comillas al inicio y al final
        size_t len = token->length;
        if (len >= 2 && token->start[0] == '"' && token->start[len - 1] == '"') {
            char *str = malloc(len - 1); // -2 por las comillas +1 para el terminador nulo
            memcpy(str, token->start + 1, len - 2);
            str[len - 2] = '\0';
            return str;
        }
    }
    // Comportamiento estándar para otros tokens
    char *str = malloc(token->length + 1);
    memcpy(str, token->start, token->length);
    str[token->length] = '\0';
    return str;
}

// Funciones auxiliares públicas del parser (alineadas con f_parser.h)
void f_parser_advance(FoxyParser *parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = f_lexer_next_token(parser->lexer);
}

bool f_parser_check(FoxyParser *parser, int token_subtype) {
    return parser->current_token.subtype == (unsigned int)token_subtype;
}

bool f_parser_match(FoxyParser *parser, int token_subtype) {
    if (f_parser_check(parser, token_subtype)) {
        f_parser_advance(parser);
        return true;
    }
    return false;
}

bool f_parser_expect(FoxyParser *parser, int token_subtype, const char *message) {
    if (f_parser_check(parser, token_subtype)) {
        f_parser_advance(parser);
        return true;
    }
    fprintf(stderr, "[Foxy Parser Error] %s\n", message);
    parser->had_error = true;
    return false;
}

// Declaraciones adelantadas
static FoxyASTNode* parse_statement(FoxyParser *parser);
static FoxyASTNode* parse_expression(FoxyParser *parser);
static FoxyASTNode* parse_primary(FoxyParser *parser);

/**
 * Parsea la expresion o instruccion 'env'
 * Sintaxis: env
 */
FoxyASTNode* f_parser_parse_env(FoxyParser *parser) {
    if (!parser) return NULL;
    f_parser_advance(parser);
    return f_ast_create_env();
}

/**
 * Parsea la creacion de un entorno compartido
 * Sintaxis: env_create( <name_expr> )
 */
FoxyASTNode* f_parser_parse_env_create(FoxyParser *parser) {
    if (!parser) return NULL;

    f_parser_advance(parser);

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_LPAREN, "Se esperaba '(' tras 'env_create'")) {
        return NULL;
    }

    FoxyASTNode *name_expr = parse_expression(parser);
    if (!name_expr) {
        f_utils_write_runtime_error(NULL, FOXY_TOKEN_ERROR_SYNTAX,
            "Se esperaba una expresion con el nombre del protocolo en 'env_create'");
        return NULL;
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_RPAREN, "Se esperaba ')' tras el argumento de 'env_create'")) {
        f_ast_node_free(name_expr);
        return NULL;
    }

    return f_ast_create_env_create(name_expr);
}

/**
 * Parsea la vinculacion de un entorno a un proceso
 * Sintaxis: env_bind( <proc_expr> , <env_expr> )
 */
FoxyASTNode* f_parser_parse_env_bind(FoxyParser *parser) {
    if (!parser) return NULL;

    f_parser_advance(parser);

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_LPAREN, "Se esperaba '(' tras 'env_bind'")) {
        return NULL;
    }

    FoxyASTNode *proc_expr = parse_expression(parser);
    if (!proc_expr) {
        f_utils_write_runtime_error(NULL, FOXY_TOKEN_ERROR_SYNTAX,
            "Se esperaba la expresion de proceso en 'env_bind'");
        return NULL;
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_COMMA, "Se esperaba ',' entre el proceso y el protocolo en 'env_bind'")) {
        f_ast_node_free(proc_expr);
        return NULL;
    }

    FoxyASTNode *env_expr = parse_expression(parser);
    if (!env_expr) {
        f_utils_write_runtime_error(NULL, FOXY_TOKEN_ERROR_SYNTAX,
            "Se esperaba la expresion de entorno en 'env_bind'");
        f_ast_node_free(proc_expr);
        return NULL;
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_RPAREN, "Se esperaba ')' tras los argumentos de 'env_bind'")) {
        f_ast_node_free(proc_expr);
        f_ast_node_free(env_expr);
        return NULL;
    }

    return f_ast_create_env_bind(proc_expr, env_expr);
}

/**
 * Parsea la invocacion para instanciar subprocesos asincronos (popen)
 * Sintaxis: popen( <callback_expr> [, <name_expr> [, <env_expr>]] )
 */
FoxyASTNode* f_parser_parse_popen(FoxyParser *parser) {
    if (!parser) return NULL;

    f_parser_advance(parser);

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_LPAREN, "Se esperaba '(' tras 'popen'")) {
        return NULL;
    }

    FoxyASTNode *callback_expr = parse_expression(parser);
    if (!callback_expr) {
        f_utils_write_runtime_error(NULL, FOXY_TOKEN_ERROR_SYNTAX,
            "Se esperaba una funcion callback como primer argumento en 'popen'");
        return NULL;
    }

    FoxyASTNode *name_expr = NULL;
    FoxyASTNode *env_expr  = NULL;

    if (f_parser_match(parser, FOXY_TOKEN_OPERATOR_COMMA)) {
        if (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_COMMA) && !f_parser_check(parser, FOXY_TOKEN_OPERATOR_RPAREN)) {
            name_expr = parse_expression(parser);
        }

        if (f_parser_match(parser, FOXY_TOKEN_OPERATOR_COMMA)) {
            if (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_RPAREN)) {
                env_expr = parse_expression(parser);
            }
        }
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_RPAREN, "Se esperaba ')' tras los argumentos de 'popen'")) {
        f_ast_node_free(callback_expr);
        if (name_expr) f_ast_node_free(name_expr);
        if (env_expr)  f_ast_node_free(env_expr);
        return NULL;
    }

    return f_ast_create_popen(callback_expr, name_expr, env_expr);
}

static FoxyASTNode* parse_include(FoxyParser *parser) {
    f_parser_advance(parser); // Consumir 'include'

    // --- DEPURACIÓN TEMPORAL ---
    /* fprintf(stderr, "[DEBUG PARSER] Token actual -> cat: %u, subtype: %u, len: %u, text: '%.*s'\n",
            parser->current_token.type_category,
            parser->current_token.subtype,
            parser->current_token.length,
            (int)parser->current_token.length,
            parser->current_token.start ? parser->current_token.start : "NULL"); */
    // ---------------------------

    char first_char = (parser->current_token.start != NULL && parser->current_token.length > 0) 
                      ? parser->current_token.start[0] : '\0';
    
    bool is_identifier = isalpha((unsigned char)first_char) || first_char == '_';
    bool is_string_literal = (first_char == '"');

    if (!is_identifier && !is_string_literal) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba una ruta válida después de include\n");
        parser->had_error = true;
        return NULL;
    }

    char *path = token_to_string(&parser->current_token);
    f_parser_advance(parser); // Consumir la ruta

    return f_ast_create_include(path);
}

static const char escape_lookup_table[256] = {
    ['a']  = '\a', // 0x07 (Alert / Bell)
    ['b']  = '\b', // 0x08 (Backspace)
    ['f']  = '\f', // 0x0C (Form Feed)
    ['n']  = '\n', // 0x0A (Line Feed)
    ['r']  = '\r', // 0x0D (Carriage Return)
    ['t']  = '\t', // 0x09 (Horizontal Tab)
    ['v']  = '\v', // 0x0B (Vertical Tab)
    ['\\'] = '\\', // 0x5C (Backslash)
    ['\''] = '\'', // 0x27 (Single Quote)
    ['\"'] = '\"', // 0x22 (Double Quote)
    ['0']  = '\0'  // 0x00 (Null Terminated)
};

static FoxyASTNode* parse_char_literal(FoxyParser *parser) {
    FoxyValue val;
    val.type = FOXY_VAL_CHAR;
    
    const char *start = parser->current_token.start;
    size_t len = parser->current_token.length;

    if (len >= 3 && start[0] == '\'') {
        if (start[1] == '\\' && len >= 4) {
            unsigned char c = (unsigned char)start[2];

            // Si es un dígito octal ('0' a '7'), procesamos el número octal
            if (c >= '0' && c <= '7') {
                char *endptr;
                // Parsea la Secuencia Octal empezando en start + 2
                val.as.ival = (int64_t)(char)strtol(start + 2, &endptr, 8);
            } else {
                // Para letras de escape estándar usa la tabla O(1)
                char translated = escape_lookup_table[c];
                val.as.ival = (int64_t)(char)(translated != 0 ? translated : c);
            }
        } else {
            val.as.ival = (int64_t)(char)start[1];
        }
    } else {
        val.as.ival = 0;
    }

    f_parser_advance(parser);
    return f_ast_create_literal(val);
}

static FoxyASTNode* parse_primary(FoxyParser *parser) {
    // 1. Literales de Enteros y Números Genéricos
    if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_INT || 
        parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_NUMBER) {
        
        FoxyValue val = {0};
        val.type = FOXY_VAL_INT;
        
        char *num_str = token_to_string(&parser->current_token);
        val.as.ival = (int64_t)strtoll(num_str, NULL, 10);
        free(num_str);
        
        FoxyASTNode *lit_node = f_ast_create_literal(val);
        f_parser_advance(parser);
        return lit_node;
    }

    // 2. Literales de Carácter (Nuevo manejo para comillas simples)
    if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_CHAR) {
        return parse_char_literal(parser);
    }

    // 3. Literales de Punto Flotante (float / double)
    if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_FLOAT || 
        parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_DOUBLE) {
        
        FoxyValue val = {0};
        char *num_str = token_to_string(&parser->current_token);

        if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_FLOAT) {
            val.type = FOXY_VAL_FLOAT;
            val.as.fval = strtof(num_str, NULL);
        } else {
            val.type = FOXY_VAL_DOUBLE;
            val.as.dval = strtod(num_str, NULL);
        }

        free(num_str);
        
        FoxyASTNode *lit_node = f_ast_create_literal(val);
        f_parser_advance(parser);
        return lit_node;
    }

    // 4. Literales de Cadenas de Caracteres / Arreglos
    if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_CHAR_ARRAY) {
        char *raw_str = token_to_string(&parser->current_token);
        size_t len = strlen(raw_str);
        char *content_src = raw_str;
        size_t content_len = len;

        if (len >= 2 && raw_str[0] == '"' && raw_str[len - 1] == '"') {
            raw_str[len - 1] = '\0';
            content_src = raw_str + 1;
            content_len = len - 2;
        }

        char *unescaped_buf = malloc(content_len + 1);
        size_t final_len = 0;

        if (unescaped_buf) {
            final_len = f_utils_unescape_string(content_src, content_len, unescaped_buf, content_len + 1);
            if (final_len == (size_t)-1) {
                strcpy(unescaped_buf, content_src);
                final_len = content_len;
            }
        } else {
            unescaped_buf = strdup(content_src);
            final_len = content_len;
        }

        FoxyValue val = f_value_create_char_array(unescaped_buf, final_len);
        
        free(raw_str);
        free(unescaped_buf);

        FoxyASTNode *lit_node = f_ast_create_literal(val);
        f_parser_advance(parser);
        return lit_node;
    }

    // 5. Identificadores, Comandos del Entorno y Llamadas a Funciones
    if (parser->current_token.subtype == FOXY_TOKEN_IDENTIFIER_NAME) {
        char *name = token_to_string(&parser->current_token);

        f_parser_advance(parser);

        // Llamada a función: nombre(...)
        if (parser->current_token.subtype == FOXY_TOKEN_OPERATOR_LPAREN) {
            FoxyASTNode *call_node = f_ast_create_call(name);
            f_parser_advance(parser); // consumir '('

            if (parser->current_token.subtype != FOXY_TOKEN_OPERATOR_RPAREN) {
                do {
                    FoxyASTNode *arg = parse_expression(parser);
                    if (arg) {
                        f_ast_call_add_arg(call_node, arg);
                    }
                    if (parser->current_token.subtype == FOXY_TOKEN_OPERATOR_COMMA) {
                        f_parser_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current_token.subtype != FOXY_TOKEN_OPERATOR_RPAREN && !parser->had_error);
            }

            if (parser->current_token.subtype == FOXY_TOKEN_OPERATOR_RPAREN) {
                f_parser_advance(parser); // consumir ')'
            } else {
                fprintf(stderr, "[Foxy Parser Error] Se esperaba ')' al final de los argumentos\n");
                parser->had_error = true;
            }

            free(name);
            return call_node;
        }

        // Variable o identificador simple
        FoxyASTNode *id_node = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
        id_node->as.identifier_node.name = name;
        return id_node;
    }

    // Fallback: Si se recibe un token desconocido o inesperado
    FoxyValue null_val = {0};
    null_val.type = FOXY_VAL_NULL;
    FoxyASTNode *fallback = f_ast_create_literal(null_val);
    f_parser_advance(parser);
    return fallback;
}

static FoxyASTNode* parse_expression(FoxyParser *parser) {
    // 1. Analizar el lado izquierdo (ej. la 'i' en 'i < 10' o 'i++')
    FoxyASTNode *left = parse_primary(parser);
    if (!left) return NULL;

    // 2. Verificar si el token actual es un operador
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR) {
        int op_subtype = parser->current_token.subtype;

        // --- Manejo especial para el incremento postfijo (i++) ---
        // Transformamos automáticamente 'i++' en una asignación: 'i = i + 1'
        if (op_subtype == FOXY_TOKEN_OPERATOR_INC) {
            f_parser_advance(parser); // Consumir '++'

            if (left->type != FOXY_AST_NODE_IDENTIFIER) {
                fprintf(stderr, "[Foxy Parser Error] El operador '++' requiere un identificador válido\n");
                parser->had_error = true;
                return NULL;
            }

            // Crear el nodo identificador para el lado izquierdo de la suma (usando identifier_node.name)
            FoxyASTNode *id_copy = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
            if (!id_copy) return NULL;
            id_copy->as.identifier_node.name = strdup(left->as.identifier_node.name);

            // Crear el literal numérico '1' (usando literal_node.value)
            FoxyValue one_val = {0};
            one_val.type = FOXY_VAL_INT; 
            one_val.as.ival = 1;
            FoxyASTNode *literal_one = f_ast_create_literal(one_val);

            // Crear la operación binaria: i + 1 (usando binary_node)
            FoxyASTNode *addition = f_ast_create_binary_op('+', id_copy, literal_one);

            // Crear el nodo de asignación: i = (i + 1) (usando binary_node)
            FoxyASTNode *assign_node = f_ast_node_new(FOXY_AST_NODE_ASSIGN);
            if (!assign_node) return NULL;
            
            assign_node->as.binary_node.left = left;       // El 'i' original
            assign_node->as.binary_node.right = addition;    // El resultado de la suma
            assign_node->as.binary_node.op_token = '=';

            return assign_node;
        }

        int op_char = 0;
        bool is_binary_op = true;

        // Mapeo limpio mediante switch-case para operadores binarios normales
        switch (op_subtype) {
            case FOXY_TOKEN_OPERATOR_LT:  op_char = '<'; break;
            case FOXY_TOKEN_OPERATOR_GT:  op_char = '>'; break;
            case FOXY_TOKEN_OPERATOR_LE:  op_char = '<'; break; 
            case FOXY_TOKEN_OPERATOR_GE:  op_char = '>'; break; 
            case FOXY_TOKEN_OPERATOR_EQ:  op_char = '='; break;
            case FOXY_TOKEN_OPERATOR_ADD: op_char = '+'; break;
            case FOXY_TOKEN_OPERATOR_SUB: op_char = '-'; break;
            case FOXY_TOKEN_OPERATOR_MUL: op_char = '*'; break;
            case FOXY_TOKEN_OPERATOR_DIV: op_char = '/'; break;
            default:
                is_binary_op = false; 
                break;
        }

        if (is_binary_op) {
            f_parser_advance(parser); // Consumir el operador

            // 3. Analizar el lado derecho
            FoxyASTNode *right = parse_primary(parser);
            if (!right) {
                fprintf(stderr, "[Foxy Parser Error] Se esperaba una expresión a la derecha del operador\n");
                parser->had_error = true;
                return NULL;
            }

            // 4. Crear y retornar el nodo de operación binaria
            return f_ast_create_binary_op(op_char, left, right);
        }
    }

    return left;
}

static FoxyASTNode* parse_for(FoxyParser *parser) {
    f_parser_advance(parser); // Consumir 'for'

    // 1. Esperar '('
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        parser->current_token.subtype != FOXY_TOKEN_OPERATOR_LPAREN) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba '(' después de 'for'\n");
        parser->had_error = true;
        return NULL;
    }
    f_parser_advance(parser); // Consumir '('

    // 2. Inicialización (Opcional, ej. int i = 0)
    FoxyASTNode *init = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
          (parser->current_token.subtype == 0 || parser->current_token.subtype == 27))) {
        init = parse_statement(parser);
    }

    // Consumir el primer ';'
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        (parser->current_token.subtype != 0 && parser->current_token.subtype != 27)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ';' después de la inicialización del 'for'\n");
        parser->had_error = true;
        return NULL;
    }
    f_parser_advance(parser); // Consumir ';'

    // 3. Condición (Opcional, ej. i < 10)
    FoxyASTNode *condition = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
          (parser->current_token.subtype == 0 || parser->current_token.subtype == 27))) {
        condition = parse_expression(parser);
    }

    // Consumir el segundo ';'
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        (parser->current_token.subtype != 0 && parser->current_token.subtype != 27)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ';' después de la condición del 'for'\n");
        parser->had_error = true;
        return NULL;
    }
    f_parser_advance(parser); // Consumir ';'

    // 4. Incremento (Opcional, ej. i = i + 1 o solo i)
    FoxyASTNode *increment = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
          parser->current_token.subtype == FOXY_TOKEN_OPERATOR_RPAREN)) {
        
        // Comprobar si es un identificador
        if (parser->current_token.type_category == FOXY_TOKEN_CAT_IDENTIFIER) {
            
            // Extraer el nombre del identificador desde el token
            char *var_name = strndup(parser->current_token.start, parser->current_token.length);
            FoxyASTNode *left_var = f_ast_create_identifier(var_name);
            free(var_name); // Liberar la cadena temporal si f_ast_create_identifier la duplica internamente
            
            f_parser_advance(parser); // Consumir el identificador (ej. 'i')

            // Verificar si el siguiente token es el operador de asignación '='
            if (parser->current_token.subtype == '=' || 
                (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && parser->current_token.subtype == 0)) {
                
                f_parser_advance(parser); // Consumir '='
                FoxyASTNode *expr = parse_expression(parser);
                
                // Crear el nodo de asignación binaria
                increment = f_ast_create_assign(left_var, expr);
            } else {
                // Si no llevaba '=', significa que el incremento era solo la variable (ej. 'i')
                increment = left_var; 
            }
        } else {
            increment = parse_expression(parser);
        }
    }

    // --- IMPRESIÓN DE DEBUG PARA EL PARÉNTESIS DE CIERRE ---
    fprintf(stderr, "[DEBUG ERROR RPAREN] Token actual encontrado -> cat: %u, subtype: %u\n",
        parser->current_token.type_category, 
        parser->current_token.subtype);

    // 5. Consumir ')'
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        (parser->current_token.subtype != 18 && parser->current_token.subtype != FOXY_TOKEN_OPERATOR_RPAREN)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ')' al final de la cabecera del 'for'\n");
        parser->had_error = true;
        return NULL;
    }
    f_parser_advance(parser); // Consumir ')'

    // 5. Cuerpo del bucle
    FoxyASTNode *body = parse_statement(parser);
    return f_ast_create_for(init, condition, increment, body);
}

static FoxyASTNode* parse_expression_statement(FoxyParser *parser) {
    FoxyASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;

    // Si tu lenguaje usa punto y coma al final de las sentencias, puedes consumirlo opcionalmente aquí:
    // if (parser->current_token.subtype == ';') { f_parser_advance(parser); }

    return f_ast_create_expr_stmt(expr);
}

static FoxyASTNode* parse_var_decl(FoxyParser *parser) {
    // 1. Consumir Tipo
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_TYPE) {
        return NULL;
    }
    f_parser_advance(parser);

    // 2. Consumir Identificador
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_IDENTIFIER) {
        return NULL;
    }
    const char *var_name = strndup(parser->current_token.start, parser->current_token.length);
    f_parser_advance(parser);

    // 3. Manejo de corchetes '[]'
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
        parser->current_token.subtype == FOXY_TOKEN_OPERATOR_LBRACKET) {
        f_parser_advance(parser);
        if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
              parser->current_token.subtype == FOXY_TOKEN_OPERATOR_RBRACKET)) {
            parse_expression(parser);
        }
        f_parser_advance(parser); // Consumir ']'
    }

    // 4. Inicializador: Comparación directa por lexema para no depender del enum exacto
    FoxyASTNode *initializer = NULL;
    if (parser->current_token.length == 1 && parser->current_token.start[0] == '=') {
        f_parser_advance(parser); // Consumir '='
        initializer = parse_expression(parser);
    }

    return f_ast_create_var_decl(var_name, initializer);
}

static FoxyASTNode* parse_statement(FoxyParser *parser) {
    // 1. Verificar si es un 'include'
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_KEYWORD && 
        parser->current_token.subtype == FOXY_TOKEN_LIST_INCLUDE) {
        return parse_include(parser);
    }

    // 2. Verificar si es un bucle 'for'
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_KEYWORD && 
        parser->current_token.subtype == FOXY_TOKEN_LIST_FOR) { 
        return parse_for(parser);
    }

    // 3. Declaraciones con tipo (int i = 0), expresiones, etc.
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_TYPE) {
        return parse_var_decl(parser);
    }

    return parse_expression_statement(parser);
}

FoxyASTNode* f_parser_parse(FoxyLexer *lexer) {
    FoxyParser parser;
    parser.lexer = lexer;
    parser.had_error = false;

    parser.current_token = f_lexer_next_token(lexer);
    parser.peek_token = f_lexer_next_token(lexer);

    FoxyASTNode *program = f_ast_create_program();

    while (parser.current_token.length > 0 && parser.current_token.start != NULL && !parser.had_error) {
        FoxyASTNode *stmt = parse_statement(&parser);
        if (stmt) {
            f_ast_program_add(program, stmt);
        } else {
            f_parser_advance(&parser);
        }
    }

    if (parser.had_error) {
        f_ast_node_free(program);
        return NULL;
    }

    return program;
}