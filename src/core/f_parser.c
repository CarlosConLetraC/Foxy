#include "f_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "f_parser.h"
#include "f_utils.h"
#include "f_ast.h"
#include "f_lexer.h"
#include "f_process.h"

// Obtener una copia en cadena (null-terminated) del lexema del token
char* f_parser_token_to_string(FoxyToken *token) {
    size_t len = token->length;
    
    // Limpieza universal: si el token está envuelto en comillas dobles o simples, quítalas
    if (len >= 2 && ((token->start[0] == '"' && token->start[len - 1] == '"') || (token->start[0] == '\'' && token->start[len - 1] == '\''))) {
        char *str = malloc(len - 1); 
        if (!str) return NULL;
        memcpy(str, token->start + 1, len - 2);
        str[len - 2] = '\0';
        return str;
    }

    // Comportamiento estándar para identificadores, números, etc.
    char *str = malloc(token->length + 1);
    if (!str) return NULL;
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
        int check_state = (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_COMMA) && !f_parser_check(parser, FOXY_TOKEN_OPERATOR_RPAREN)) ? 1 : 0;
        switch (check_state) {
            case 1:
                name_expr = parse_expression(parser);
                break;
            default:
                break;
        }

        if (f_parser_match(parser, FOXY_TOKEN_OPERATOR_COMMA)) {
            int env_check = (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_RPAREN)) ? 1 : 0;
            switch (env_check) {
                case 1:
                    env_expr = parse_expression(parser);
                    break;
                default:
                    break;
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
    char first_char = (parser->current_token.start != NULL && parser->current_token.length > 0) ? parser->current_token.start[0] : '\0';
    bool is_identifier = isalpha((unsigned char)first_char) || first_char == '_';
    bool is_string_literal = (first_char == '"');
    if (!is_identifier && !is_string_literal) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba una ruta válida después de include\n");
        parser->had_error = true;
        return NULL;
    }
    char *path = f_parser_token_to_string(&parser->current_token);
    f_parser_advance(parser); // Consumir la ruta

    FoxyASTNode *node = f_ast_create_include(path);
    // f_ast_create_include asume la propiedad o duplica según convenga, 
    // liberamos el temporal local si ya no se usa directamente.
    free(path); 
    return node;
}

static const char escape_lookup_table[256] = {
    ['a']  = '\a', 
    ['b']  = '\b', 
    ['f']  = '\f', 
    ['n']  = '\n', 
    ['r']  = '\r', 
    ['t']  = '\t', 
    ['v']  = '\v', 
    ['\\'] = '\\', 
    ['\''] = '\'', 
    ['\"'] = '\"', 
    ['0']  = '\0'  
};

static FoxyASTNode* parse_char_literal(FoxyParser *parser) {
    FoxyValue val;
    val.type = FOXY_VAL_CHAR;
    
    const char *start = parser->current_token.start;
    size_t len = parser->current_token.length;

    if (len >= 3 && start[0] == '\'') {
        int char_type = (start[1] == '\\' && len >= 4) ? 1 : 2;
        switch (char_type) {
            case 1: {
                unsigned char c = (unsigned char)start[2];
                int is_octal = (c >= '0' && c <= '7') ? 1 : 0;
                switch (is_octal) {
                    case 1: {
                        char *endptr;
                        val.as.ival = (int64_t)(char)strtol(start + 2, &endptr, 8);
                        break;
                    }
                    default: {
                        char translated = escape_lookup_table[c];
                        val.as.ival = (int64_t)(char)(translated != 0 ? translated : c);
                        break;
                    }
                }
                break;
            }
            case 2:
                val.as.ival = (int64_t)(char)start[1];
                break;
        }
    } else {
        val.as.ival = 0;
    }

    f_parser_advance(parser);
    return f_ast_create_literal(val);
}

static FoxyASTNode* parse_block(FoxyParser *parser) {
    FoxyASTNode *block_node = f_ast_node_new(FOXY_AST_NODE_BLOCK);
    block_node->as.block_node.capacity = 8;
    block_node->as.block_node.count = 0;
    block_node->as.block_node.statements = malloc(sizeof(FoxyASTNode*) * block_node->as.block_node.capacity);

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_LBRACE, "Se esperaba '{' al inicio del bloque")) {
        free(block_node->as.block_node.statements);
        free(block_node);
        return NULL;
    }

    while (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_RBRACE) && 
           parser->current_token.length > 0 && 
           !parser->had_error) {
           
        FoxyASTNode *stmt = parse_statement(parser);
        if (stmt) {
            if (block_node->as.block_node.count >= block_node->as.block_node.capacity) {
                block_node->as.block_node.capacity *= 2;
                block_node->as.block_node.statements = realloc(
                    block_node->as.block_node.statements, 
                    sizeof(FoxyASTNode*) * block_node->as.block_node.capacity
                );
            }
            block_node->as.block_node.statements[block_node->as.block_node.count++] = stmt;
        } else {
            f_parser_advance(parser);
        }
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_RBRACE, "Se esperaba '}' al final del bloque")) {
        f_ast_node_free(block_node);
        return NULL;
    }

    return block_node;
}

static FoxyASTNode* parse_function(FoxyParser *parser) {
    f_parser_advance(parser); // Consumir 'function'

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_IDENTIFIER) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba el nombre de la función\n");
        parser->had_error = true;
        return NULL;
    }

    char *func_name = strndup(parser->current_token.start, parser->current_token.length);
    f_parser_advance(parser);

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_LPAREN, "Se esperaba '(' tras el nombre de la función")) {
        free(func_name);
        return NULL;
    }

    size_t param_cap = 4;
    size_t param_count = 0;
    char **param_names = malloc(sizeof(char*) * param_cap);

    while (!f_parser_check(parser, FOXY_TOKEN_OPERATOR_RPAREN) && !parser->had_error) {
        if (parser->current_token.type_category == FOXY_TOKEN_CAT_TYPE) {
            f_parser_advance(parser);
        }

        if (parser->current_token.type_category != FOXY_TOKEN_CAT_IDENTIFIER) {
            fprintf(stderr, "[Foxy Parser Error] Se esperaba el nombre del parámetro\n");
            parser->had_error = true;
            break;
        }

        char *p_name = strndup(parser->current_token.start, parser->current_token.length);
        f_parser_advance(parser);

        if (param_count >= param_cap) {
            param_cap *= 2;
            param_names = realloc(param_names, sizeof(char*) * param_cap);
        }
        param_names[param_count++] = p_name;

        if (f_parser_check(parser, FOXY_TOKEN_OPERATOR_COMMA)) {
            f_parser_advance(parser);
        } else {
            break;
        }
    }

    if (!f_parser_expect(parser, FOXY_TOKEN_OPERATOR_RPAREN, "Se esperaba ')' al final de los parámetros")) {
        for (size_t i = 0; i < param_count; i++) free(param_names[i]);
        free(param_names);
        free(func_name);
        return NULL;
    }

    FoxyASTNode *body = parse_block(parser);
    if (!body) {
        for (size_t i = 0; i < param_count; i++) free(param_names[i]);
        free(param_names);
        free(func_name);
        return NULL;
    }

    FoxyASTNode *node = f_ast_node_new(FOXY_AST_NODE_FUNCTION);
    node->as.function_node.name = func_name;
    node->as.function_node.param_names = param_names;
    node->as.function_node.param_count = param_count;
    node->as.function_node.body = body;
    return node;
}

static FoxyASTNode* parse_primary(FoxyParser *parser) {
    switch (parser->current_token.subtype) {
        case FOXY_TOKEN_TYPE_BOOL: {
            char *val_str = f_parser_token_to_string(&parser->current_token);
            bool is_true = (strcmp(val_str, "true") == 0);
            free(val_str);

            FoxyValue val = {0};
            val.type = FOXY_VAL_BOOL;
            val.as.boolean = is_true;

            FoxyASTNode *lit_node = f_ast_create_literal(val);
            f_parser_advance(parser);
            return lit_node;
        }

        case FOXY_TOKEN_IDENTIFIER_INT:
        case FOXY_TOKEN_IDENTIFIER_NUMBER: {
            FoxyValue val = {0};
            val.type = FOXY_VAL_INT;
            
            char *num_str = f_parser_token_to_string(&parser->current_token);
            val.as.ival = (int64_t)strtoll(num_str, NULL, 10);
            free(num_str);
            
            FoxyASTNode *lit_node = f_ast_create_literal(val);
            f_parser_advance(parser);
            return lit_node;
        }

        case FOXY_TOKEN_IDENTIFIER_CHAR: {
            return parse_char_literal(parser);
        }

        case FOXY_TOKEN_IDENTIFIER_FLOAT:
        case FOXY_TOKEN_IDENTIFIER_DOUBLE: {
            FoxyValue val = {0};
            char *num_str = f_parser_token_to_string(&parser->current_token);

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

        case FOXY_TOKEN_TYPE_OBJECT: {
            char *raw_str = f_parser_token_to_string(&parser->current_token);
            size_t len = strlen(raw_str);
            char *content_src = raw_str;
            size_t content_len = len;

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

        case FOXY_TOKEN_IDENTIFIER_NAME: {
            char *name = f_parser_token_to_string(&parser->current_token);

            f_parser_advance(parser);

            if (parser->current_token.subtype == FOXY_TOKEN_OPERATOR_LPAREN) {
                FoxyASTNode *call_node = f_ast_create_call(name);
                f_parser_advance(parser); 

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
                    f_parser_advance(parser); 
                } else {
                    fprintf(stderr, "[Foxy Parser Error] Se esperaba ')' al final de los argumentos\n");
                    parser->had_error = true;
                }

                free(name);
                return call_node;
            }

            FoxyASTNode *id_node = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
            id_node->as.identifier_node.name = name;
            return id_node;
        }

        default: {
            FoxyValue null_val = {0};
            null_val.type = FOXY_VAL_NULL;
            FoxyASTNode *fallback = f_ast_create_literal(null_val);
            f_parser_advance(parser);
            return fallback;
        }
    }
}

static FoxyASTNode* parse_expression(FoxyParser *parser) {
    bool is_negative = false;
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
        parser->current_token.subtype == FOXY_TOKEN_OPERATOR_SUB) {
        is_negative = true;
        f_parser_advance(parser); 
    }

    FoxyASTNode *left = parse_primary(parser);
    if (!left) return NULL;

    if (is_negative && left->type == FOXY_AST_NODE_LITERAL) {
        switch (left->as.literal_node.value.type) {
            case FOXY_VAL_INT:
            case FOXY_VAL_LONG:
            case FOXY_VAL_LONG_LONG:
            case FOXY_VAL_UNSIGNED_LONG_LONG:
                left->as.literal_node.value.as.ival = -left->as.literal_node.value.as.ival;
                break;
            case FOXY_VAL_NUMBER:
            case FOXY_VAL_FLOAT:
            case FOXY_VAL_DOUBLE:
                left->as.literal_node.value.as.dval = -left->as.literal_node.value.as.dval;
                break;
            default:
                break;
        }
    }

    if (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR) {
        int op_subtype = parser->current_token.subtype;

        if (op_subtype == FOXY_TOKEN_OPERATOR_INC) {
            f_parser_advance(parser); 

            if (left->type != FOXY_AST_NODE_IDENTIFIER) {
                fprintf(stderr, "[Foxy Parser Error] El operador '++' requiere un identificador válido\n");
                parser->had_error = true;
                return NULL;
            }

            FoxyASTNode *id_copy = f_ast_node_new(FOXY_AST_NODE_IDENTIFIER);
            if (!id_copy) return NULL;
            id_copy->as.identifier_node.name = strdup(left->as.identifier_node.name);

            FoxyValue one_val = {0};
            one_val.type = FOXY_VAL_INT; 
            one_val.as.ival = 1;
            FoxyASTNode *literal_one = f_ast_create_literal(one_val);

            FoxyASTNode *addition = f_ast_create_binary_op('+', id_copy, literal_one);

            FoxyASTNode *assign_node = f_ast_node_new(FOXY_AST_NODE_ASSIGN);
            if (!assign_node) {
                f_ast_node_free(addition);
                f_ast_node_free(id_copy);
                return NULL;
            }
            
            assign_node->as.binary_node.left = left;       
            assign_node->as.binary_node.right = addition;    
            assign_node->as.binary_node.op_token = '=';

            return assign_node;
        }

        int op_char = 0;
        bool is_binary_op = true;

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
            f_parser_advance(parser); 

            FoxyASTNode *right = parse_primary(parser);
            if (!right) {
                fprintf(stderr, "[Foxy Parser Error] Se esperaba una expresión a la derecha del operador\n");
                parser->had_error = true;
                return NULL;
            }

            return f_ast_create_binary_op(op_char, left, right);
        }
    }

    return left;
}

static FoxyASTNode* parse_for(FoxyParser *parser) {
    f_parser_advance(parser); 

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        parser->current_token.subtype != FOXY_TOKEN_OPERATOR_LPAREN) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba '(' después de 'for'\n");
        parser->had_error = true;
        return NULL;
    }
    f_parser_advance(parser); 

    FoxyASTNode *init = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && (parser->current_token.subtype == 0 || parser->current_token.subtype == 27)))
        init = parse_statement(parser);

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        (parser->current_token.subtype != 0 && parser->current_token.subtype != 27)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ';' después de la inicialización del 'for'\n");
        parser->had_error = true;
        if (init) f_ast_node_free(init);
        return NULL;
    }
    f_parser_advance(parser); 

    FoxyASTNode *condition = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && (parser->current_token.subtype == 0 || parser->current_token.subtype == 27)))
        condition = parse_expression(parser);

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || 
        (parser->current_token.subtype != 0 && parser->current_token.subtype != 27)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ';' después de la condición del 'for'\n");
        parser->had_error = true;
        if (init) f_ast_node_free(init);
        if (condition) f_ast_node_free(condition);
        return NULL;
    }
    f_parser_advance(parser); 

    FoxyASTNode *increment = NULL;
    if (!(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && parser->current_token.subtype == FOXY_TOKEN_OPERATOR_RPAREN)) {
        if (parser->current_token.type_category == FOXY_TOKEN_CAT_IDENTIFIER) {
            char *var_name = strndup(parser->current_token.start, parser->current_token.length);
            FoxyASTNode *left_var = f_ast_create_identifier(var_name);
            free(var_name); 
            
            f_parser_advance(parser); 

            if (parser->current_token.subtype == '=' || (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && parser->current_token.subtype == 0)) {
                f_parser_advance(parser); 
                FoxyASTNode *expr = parse_expression(parser);
                increment = f_ast_create_assign(left_var, expr);
            } else {
                increment = left_var; 
            }
        } else {
            increment = parse_expression(parser);
        }
    }

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_OPERATOR || (parser->current_token.subtype != 18 && parser->current_token.subtype != FOXY_TOKEN_OPERATOR_RPAREN)) {
        fprintf(stderr, "[Foxy Parser Error] Se esperaba ')' al final de la cabecera del 'for'\n");
        parser->had_error = true;
        if (init) f_ast_node_free(init);
        if (condition) f_ast_node_free(condition);
        if (increment) f_ast_node_free(increment);
        return NULL;
    }
    f_parser_advance(parser); 

    FoxyASTNode *body = parse_statement(parser);
    return f_ast_create_for(init, condition, increment, body);
}

static FoxyASTNode* parse_expression_statement(FoxyParser *parser) {
    FoxyASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    return f_ast_create_expr_stmt(expr);
}

static FoxyASTNode* parse_var_decl(FoxyParser *parser) {
    if (parser->current_token.type_category != FOXY_TOKEN_CAT_TYPE) {
        return NULL;
    }
    f_parser_advance(parser);

    if (parser->current_token.type_category != FOXY_TOKEN_CAT_IDENTIFIER) {
        return NULL;
    }
    char *var_name = strndup(parser->current_token.start, parser->current_token.length);
    f_parser_advance(parser);

    if (parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
        parser->current_token.subtype == FOXY_TOKEN_OPERATOR_LBRACKET) {
        f_parser_advance(parser);
        int not_rbracket = !(parser->current_token.type_category == FOXY_TOKEN_CAT_OPERATOR && 
                             parser->current_token.subtype == FOXY_TOKEN_OPERATOR_RBRACKET);
        switch (not_rbracket) {
            case 1: {
                FoxyASTNode *array_size_expr = parse_expression(parser);
                if (array_size_expr) f_ast_node_free(array_size_expr);
                break;
            }
            default:
                break;
        }
        f_parser_advance(parser); 
    }

    FoxyASTNode *initializer = NULL;
    if (parser->current_token.length == 1 && parser->current_token.start[0] == '=') {
        f_parser_advance(parser); 
        initializer = parse_expression(parser);
    }

    FoxyASTNode *var_decl_node = f_ast_create_var_decl(var_name, initializer);
    free(var_name); // Liberar la copia local ya que f_ast_create_var_decl realiza su propia gestión/duplicación interna
    return var_decl_node;
}

static FoxyASTNode* parse_statement(FoxyParser *parser) {
    if (parser->current_token.type_category == FOXY_TOKEN_CAT_KEYWORD && parser->current_token.subtype == FOXY_TOKEN_LIST_INCLUDE)
        return parse_include(parser);

    if (parser->current_token.type_category == FOXY_TOKEN_CAT_KEYWORD && parser->current_token.subtype == FOXY_TOKEN_LIST_FOR)
        return parse_for(parser);

    if (parser->current_token.type_category == FOXY_TOKEN_CAT_KEYWORD && parser->current_token.subtype == FOXY_TOKEN_LIST_FUNCTION)
        return parse_function(parser);

    if (parser->current_token.type_category == FOXY_TOKEN_CAT_TYPE)
        return parse_var_decl(parser);

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