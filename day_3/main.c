// Homework for Bitwise, day 3
// https://github.com/pervognsen/bitwise

#include <assert.h>
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
// Virtual machine
////////////////////////////////////////////////////////////////////////////////

enum vm_opcode_t
{
    VM_OPCODE_NOP,
    VM_OPCODE_ADD,
    VM_OPCODE_SUB,
    VM_OPCODE_MULT,
    VM_OPCODE_DIV,
    VM_OPCODE_POW,
    VM_OPCODE_INV,
    VM_OPCODE_LIT,
    VM_OPCODE_HALT
};
typedef enum vm_opcode_t vm_opcode_t;

enum
{
    VM_STACK_SIZE = 1024
};

int32_t vm_stack[VM_STACK_SIZE];
int32_t * vm_stack_top = vm_stack;

void vm_push(int32_t value)
{
    assert(vm_stack_top < vm_stack + VM_STACK_SIZE);
    *vm_stack_top++ = value;
}

int32_t vm_pop()
{
    assert(vm_stack_top > vm_stack);
    return *--vm_stack_top;
}

void vm_execute(uint8_t * code)
{
    bool running = true;
    while (running)
    {
        vm_opcode_t op = *code++;

        switch (op)
        {
        case VM_OPCODE_HALT:
            running = false;
            break;
        case VM_OPCODE_LIT:
            {
                int32_t value = 0;
                value |= *code++;
                value |= (*code++) << 8; 
                value |= (*code++) << 16; 
                value |= (*code++) << 24;
                vm_push(value);
            }
            break;
        case VM_OPCODE_INV:
            vm_push(-vm_pop());
            break;
        case VM_OPCODE_ADD:
            vm_push(vm_pop() + vm_pop());
            break;
        case VM_OPCODE_SUB:
            vm_push(-vm_pop() + vm_pop());
            break;
        case VM_OPCODE_MULT:
            vm_push(vm_pop() * vm_pop());
            break;
        case VM_OPCODE_DIV:
            {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a / b);
            }
            break;
        case VM_OPCODE_POW:
            {
                int32_t exponent = vm_pop();
                int32_t value = vm_pop();
                int32_t result = 1;
                bool must_invert = false;
                if (exponent < 0)
                {
                    exponent = -exponent;
                    must_invert = true;
                }
                while (exponent-- > 0)
                {
                    result *= value;
                }
                if (must_invert)
                {
                    result = 1 / result;
                }
                vm_push(result);
            }
            break;
        default:
            assert(0);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Compiler
////////////////////////////////////////////////////////////////////////////////

enum
{
    COMPILER_CODE_MAX_SIZE = 1024
};

int8_t code[COMPILER_CODE_MAX_SIZE];
int8_t * code_current = code;

void emit_opcode(vm_opcode_t opcode)
{
    assert(code_current + 1 <= code + COMPILER_CODE_MAX_SIZE);
    *code_current++ = opcode;
}

void emit_int32(int32_t value)
{
    assert(code_current + 4 <= code + COMPILER_CODE_MAX_SIZE);
    *code_current++ = value & 0xff;
    *code_current++ = (value >> 8) & 0xff;
    *code_current++ = (value >> 16) & 0xff;
    *code_current++ = (value >> 24) & 0xff;
}

#define OPCODE(X, Y) case AST_BIN_OP_##X: return VM_OPCODE_##Y;
vm_opcode_t ast_bin_op_to_vm_opcode(ast_bin_op_t op)
{
    switch (op)
    {
        OPCODE(ADD, ADD)
        OPCODE(SUBTRACT, SUB)
        OPCODE(MULTIPLY, MULT)
        OPCODE(DIVIDE, DIV)
        OPCODE(EXPONENT, POW)
    }
    return VM_OPCODE_NOP;
}
#undef OPCODE

#define OPCODE(X, Y) case AST_UN_OP_##X: return VM_OPCODE_##Y;
vm_opcode_t ast_un_op_to_vm_opcode(ast_un_op_t op)
{
    switch (op)
    {
        OPCODE(INVERT, INV)
    }
    return VM_OPCODE_NOP;
}
#undef OPCODE

void compile_inner(ast_node_t * node)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_LITERAL:
        emit_opcode(VM_OPCODE_LIT);
        emit_int32(node->literal);
        break;
    case AST_NODE_TYPE_UNARY:
        compile_inner(node->unary.child);
        emit_opcode(ast_un_op_to_vm_opcode(node->unary.operator));
        break;
    case AST_NODE_TYPE_BINARY:
        compile_inner(node->binary.left);
        compile_inner(node->binary.right);
        emit_opcode(ast_bin_op_to_vm_opcode(node->binary.operator));
        break;
    }
}

void compile(ast_node_t * node)
{
    compile_inner(node);
    emit_opcode(VM_OPCODE_HALT);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    input_stream = "-50 + -(3 + 3)";

    next_token();
    ast_node_t * ast_root = parse_expr();

    compile(ast_root);
    vm_execute(code);
    printf("%d\n", vm_pop());

    return 0;
}
