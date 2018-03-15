// Homework for Bitwise, day 2
// https://github.com/pervognsen/bitwise

// Left to do:
//      Table-driver parsing

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void * xmalloc(size_t size)
{
    void * ptr = malloc(size);
    if (!ptr)
    {
        printf("xmalloc failed\n");
        exit(0);
    }
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////
// Tokenizer
////////////////////////////////////////////////////////////////////////////////

enum token_type_t
{
    _TOKEN_TYPE_LAST_ASCII = 127,
    TOKEN_TYPE_NUMBER
};
typedef enum token_type_t token_type_t;

struct token_t
{
    token_type_t type;
    union
    {
        int32_t value;
    };
};
typedef struct token_t token_t;

const char * input_stream;
token_t current_token;

void skip_whitespaces()
{
    while (isspace(*input_stream))
    {
        input_stream++;
    }
}

void next_token()
{
    skip_whitespaces();
    
    if (isdigit(*input_stream))
    {
        current_token.type = TOKEN_TYPE_NUMBER;
        current_token.value = 0;

        while (isdigit(*input_stream))
        {
            current_token.value = current_token.value * 10 + (*input_stream - '0');
            input_stream++;
        }
    }
    else
    {
        current_token.type = *input_stream;
        input_stream++;
    }
}

////////////////////////////////////////////////////////////////////////////////
// AST
////////////////////////////////////////////////////////////////////////////////

enum ast_bin_op_t
{
    AST_BIN_OP_ADD,
    AST_BIN_OP_SUBTRACT,
    AST_BIN_OP_MULTIPLY,
    AST_BIN_OP_DIVIDE,
    AST_BIN_OP_EXPONENT
};
typedef enum ast_bin_op_t ast_bin_op_t;

enum ast_un_op_t
{
    AST_UN_OP_INVERT
};
typedef enum ast_un_op_t ast_un_op_t;

enum ast_node_type_t
{
    AST_NODE_TYPE_UNARY,
    AST_NODE_TYPE_BINARY,
    AST_NODE_TYPE_LITERAL
};
typedef enum ast_node_type_t ast_node_type_t;

struct ast_node_t
{
    ast_node_type_t type;
    union
    {
        struct
        {
            ast_un_op_t operator;
            struct ast_node_t *child;
        } unary;

        struct
        {
            ast_bin_op_t operator;
            struct ast_node_t *left;
            struct ast_node_t *right;
        } binary;

        int32_t literal;
    };
};
typedef struct ast_node_t ast_node_t;

ast_node_t * make_ast_node(ast_node_type_t type)
{
    ast_node_t * node = (ast_node_t *)xmalloc(sizeof(ast_node_t));
    node->type = type;
    return node;
}

void print_ast_tree(ast_node_t * node);

void print_ast_binary_node(ast_node_t * node)
{
    char operator_char = ' ';
    switch (node->binary.operator)
    {
    case AST_BIN_OP_MULTIPLY:
        operator_char = '*';
        break;
    case AST_BIN_OP_DIVIDE:
        operator_char = '/';
        break;
    case AST_BIN_OP_ADD:
        operator_char = '+';
        break;
    case AST_BIN_OP_SUBTRACT:
        operator_char = '-';
        break;
    case AST_BIN_OP_EXPONENT:
        operator_char = '^';
        break;
    }

    printf("(%c ", operator_char);
    print_ast_tree(node->binary.left);
    printf(" ");
    print_ast_tree(node->binary.right);
    printf(")");
}

void print_ast_unary_node(ast_node_t * node)
{
    if (node->unary.operator == AST_UN_OP_INVERT)
    {
        printf("(- ");
        print_ast_tree(node->unary.child);
        printf(")");
    }
}

void print_ast_tree(ast_node_t * node)
{
    if (!node) return;

    switch (node->type)
    {
    case AST_NODE_TYPE_LITERAL:
        printf("%d", node->literal);
        break;
    case AST_NODE_TYPE_BINARY:
        print_ast_binary_node(node);
        break;
    case AST_NODE_TYPE_UNARY:
        print_ast_unary_node(node);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Parser
////////////////////////////////////////////////////////////////////////////////

// Grammar:
// expr         = add;
// add          = mult, {('+' | '-'), mult};
// mult         = pow, {('*' | '/'), pow};
// pow          = minus, {'^', pow};
// minus        = ['-'], term;
// term         = <NUMBER> | '(', expr, ')';

bool is_token(token_type_t type)
{
    return current_token.type == type;
}

void expect_token(token_type_t type)
{
    if (current_token.type != type)
    {
        printf("Unexpected token\n");
        exit(0);
    }
}

ast_node_t * parse_expr();

ast_node_t * parse_term()
{
    if (is_token(TOKEN_TYPE_NUMBER))
    {
        ast_node_t * node = make_ast_node(AST_NODE_TYPE_LITERAL);
        node->literal = current_token.value;
        next_token();
        return node;
    }
    else
    {
        expect_token('(');
        next_token();
        ast_node_t * node = parse_expr();
        expect_token(')');
        next_token();
        return node;
    }
}

ast_node_t * parse_minus()
{
    ast_node_t * node = NULL;

    if (is_token('-'))
    {
        next_token();
        node = make_ast_node(AST_NODE_TYPE_UNARY);
        node->unary.operator = AST_UN_OP_INVERT;
        node->unary.child = NULL;
    }

    ast_node_t * sub_node = parse_term(); 

    if (!node)
    {
        node = sub_node;
    }
    else
    {
        node->unary.child = sub_node;
    }
    
    return node;
}

ast_node_t * parse_pow()
{
    ast_node_t * left_node = parse_minus();
    while (is_token('^'))
    {
        ast_node_t * op_node = make_ast_node(AST_NODE_TYPE_BINARY);
        op_node->binary.operator = AST_BIN_OP_EXPONENT;
        op_node->binary.left = left_node;
        next_token();
        op_node->binary.right = parse_pow();
        left_node = op_node;
    }
    return left_node;
}

ast_node_t * parse_mult()
{
    ast_node_t * left_node = parse_pow();
    while (is_token('*') || is_token('/'))
    {
        ast_node_t * op_node = make_ast_node(AST_NODE_TYPE_BINARY);
        op_node->binary.operator = is_token('*') ? AST_BIN_OP_MULTIPLY : AST_BIN_OP_DIVIDE;
        op_node->binary.left = left_node;
        next_token();
        op_node->binary.right = parse_pow();
        left_node = op_node;
    }
    return left_node;
}

ast_node_t * parse_add()
{
    ast_node_t * left_node = parse_mult();
    while (is_token('+') || is_token('-'))
    {
        ast_node_t * op_node = make_ast_node(AST_NODE_TYPE_BINARY);
        op_node->binary.operator = is_token('+') ? AST_BIN_OP_ADD : AST_BIN_OP_SUBTRACT;
        op_node->binary.left = left_node;
        next_token();
        op_node->binary.right = parse_mult();
        left_node = op_node;
    }
    return left_node;
}

ast_node_t * parse_expr()
{
    return parse_add();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    input_stream = "(1 + 2 + 3) * (3 + -7 / 4) - 1 ^ 2 ^ 3";
    printf("%s\n", input_stream);

    next_token();
    ast_node_t * ast_root = parse_expr();
    print_ast_tree(ast_root);

    return 0;
}
