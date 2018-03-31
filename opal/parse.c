#include "parse.h"
#include "common.h"
#include "lex.h"
#include "ast.h"
#include <stdio.h>
#include <stdbool.h>

lexer_t l;

bool is_token(token_type_t type)
{
    return l.token.type == type;
}

void expect_token(token_type_t type)
{
    if (!is_token(type))
    {
        printf("Unexpected token %d\n", type);
        exit(1); // @Todo
    }
}

bool is_token_cmp_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_CMP_START_ && type < TOKEN_TYPE_CMP_END_; 
}

bool is_token_add_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_ADD_START_ && type < TOKEN_TYPE_ADD_END_; 
}

bool is_token_mult_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_MULT_START_ && type < TOKEN_TYPE_MULT_END_; 
}

bool is_token_unary_op(void)
{
    token_type_t op = l.token.type;
    return op == TOKEN_TYPE_PLUS
        || op == TOKEN_TYPE_MINUS
        || op == TOKEN_TYPE_LOGIC_NOT
        || op == TOKEN_TYPE_NOT
        || op == TOKEN_TYPE_AND
        || op == TOKEN_TYPE_MULT;
}

// expr_list =     expr (',' expr)*
//
// operand =         NAME
//                 | STRING
//                 | INTEGER
//                 | FLOAT
//                 | '(' expr ')'
//                 | type? '{' expr_list? '}'
//                 | cast '(' type ',' expr ')'
// 
// invoke_e =      operand ('(' expr_list? ')' | '[' expr ']' | '.' NAME)*

ast_expr_t * parse_expr_invoke(void)
{
    // @Todo
    return NULL;
}

ast_expr_t * parse_expr_unary(void)
{
    ast_expr_t * expr = NULL;
    ast_expr_t * current = NULL;
    
    while (is_token_unary_op())
    {
        token_type_t op = l.token.type;
        next_token(&l);
        ast_expr_t * unary = ast_new_expr(AST_EXPR_UNARY_OP);
        unary->unary.op = op;
        unary->unary.expr = NULL;
        
        if (current) { current->unary.expr = unary; }
        if (!expr) { expr = unary; }

        current = unary;
    }

    ast_expr_t * sub_expr = parse_expr_invoke();
    if (!expr) { expr = sub_expr; }
    else { current->unary.expr = sub_expr; }

    return expr;
}

ast_expr_t * parse_expr_mult(void)
{
    ast_expr_t * expr = parse_expr_unary();
    while (is_token_mult_op())
    {
        token_type_t op = l.token.type;
        next_token(&l);
        ast_expr_t * mult_expr = ast_new_expr(AST_EXPR_BINARY_OP);
        mult_expr->binary.op = op;
        mult_expr->binary.left = expr;
        mult_expr->binary.right = parse_expr_unary();
        expr = mult_expr;
    }
    return expr;
}

ast_expr_t * parse_expr_add(void)
{
    ast_expr_t * expr = parse_expr_mult();
    while (is_token_add_op())
    {
        token_type_t op = l.token.type;
        next_token(&l);
        ast_expr_t * add_expr = ast_new_expr(AST_EXPR_BINARY_OP);
        add_expr->binary.op = op;
        add_expr->binary.left = expr;
        add_expr->binary.right = parse_expr_mult();
        expr = add_expr;
    }
    return expr;
}

ast_expr_t * parse_expr_cmp(void)
{
    ast_expr_t * expr = parse_expr_add();
    while (is_token_cmp_op())
    {
        token_type_t op = l.token.type;
        next_token(&l);
        ast_expr_t * cmp_expr = ast_new_expr(AST_EXPR_BINARY_OP);
        cmp_expr->binary.op = op;
        cmp_expr->binary.left = expr;
        cmp_expr->binary.right = parse_expr_add();
        expr = cmp_expr;
    }
    return expr;
}

ast_expr_t * parse_expr_and(void)
{
    ast_expr_t * expr = parse_expr_cmp();
    while (is_token(TOKEN_TYPE_LOGIC_AND))
    {
        next_token(&l);
        ast_expr_t * cmp_expr = ast_new_expr(AST_EXPR_BINARY_OP);
        cmp_expr->binary.op = TOKEN_TYPE_LOGIC_AND;
        cmp_expr->binary.left = expr;
        cmp_expr->binary.right = parse_expr_cmp();
        expr = cmp_expr;
    }
    return expr;
}

ast_expr_t * parse_expr_or(void)
{
    ast_expr_t * expr = parse_expr_and();
    while (is_token(TOKEN_TYPE_LOGIC_OR))
    {
        next_token(&l);
        ast_expr_t * or_expr = ast_new_expr(AST_EXPR_BINARY_OP);
        or_expr->binary.op = TOKEN_TYPE_LOGIC_OR;
        or_expr->binary.left = expr;
        or_expr->binary.right = parse_expr_and();
        expr = or_expr;
    }
    return expr;
}

ast_expr_t * parse_expr_tern(void)
{
    ast_expr_t * expr = parse_expr_or();
    if (is_token('?'))
    {
        ast_expr_t * tern_expr = ast_new_expr(AST_EXPR_TERNARY);
        tern_expr->ternary.condition = expr;
        expr = tern_expr;

        next_token(&l);
        tern_expr->ternary.then_expr = parse_expr_tern();
        expect_token(':');

        next_token(&l);
        tern_expr->ternary.else_expr = parse_expr_tern();
    }
    return expr;
}

ast_expr_t * parse_expr(void)
{
    return parse_expr_tern();
}

ast_typespec_t * parse_typespec(void);
ast_typespec_t * parse_base_type_typespec(void)
{
    ast_typespec_t * typespec = NULL;

    if (is_token(TOKEN_TYPE_IDENTIFIER))
    {
        typespec = ast_new_typespec(AST_TYPESPEC_NAME);
        typespec->name          = l.token.identifier;
        next_token(&l);
    }
    else if (is_token('('))
    {
        next_token(&l);
        typespec = parse_typespec();
        expect_token(')');
        next_token(&l);
    }
    else if (is_token(TOKEN_TYPE_KW_FN))
    {
        typespec = ast_new_typespec(AST_TYPESPEC_FN);
        typespec->fn.args           = NULL;
        typespec->fn.num_args       = 0;
        typespec->fn.return_type    = NULL;

        next_token(&l);
        expect_token('(');
        next_token(&l);

        bool has_args = !is_token(')');
        while (has_args)
        {
            ast_typespec_t * type = parse_typespec(); 
            sb_push(typespec->fn.args, type);
            typespec->fn.num_args++;

            if (is_token(','))
            {
                next_token(&l);
                has_args = true;
            }
            else
            {
                has_args = false;
            }
        }

        expect_token(')');
        next_token(&l);

        if (is_token(':'))
        {
            next_token(&l);
            typespec->fn.return_type = parse_typespec();
        }
    }
    else
    {
        printf("Unexpected token\n");
    }

    return typespec;
}

ast_typespec_t * parse_typespec(void)
{
    ast_typespec_t * type = parse_base_type_typespec();

    while (is_token('*') || is_token('['))
    {
        ast_typespec_t * parent_type = ast_new_typespec(AST_TYPESPEC_POINTER);

        if (is_token('*'))
        {
            next_token(&l);
            parent_type->type = AST_TYPESPEC_POINTER;
            parent_type->pointer.base = type;
            type = parent_type;
        }
        else if (is_token('['))
        {
            next_token(&l);
            parent_type->type = AST_TYPESPEC_ARRAY;
            parent_type->array.base = type;
            parent_type->array.size_expr = parse_expr();

            expect_token(']');
            next_token(&l);
            type = parent_type;
        }
    }

    return type;
}

ast_decl_t * parse_enum_decl(void)
{
    return NULL; // @Todo
}

ast_decl_t * parse_aggregate_decl(ast_decl_type_t type)
{
    return NULL; // @Todo
}

ast_decl_t * parse_const_var_decl(ast_decl_type_t type)
{
    return NULL; // @Todo
}

ast_decl_t * parse_type_decl(void)
{
    expect_token(TOKEN_TYPE_IDENTIFIER);
    const char * name = l.token.identifier;

    next_token(&l);
    expect_token('=');
    next_token(&l);

    ast_decl_t * decl = ast_new_decl(AST_DECL_TYPE);
    decl->name              = name;
    decl->type_decl.type    = parse_typespec();
    return decl;
}

ast_decl_t * parse_fn_decl(void)
{
    return NULL; // @Todo
}


sb_t(ast_decl_t *) parse_document(void)
{
    sb_t(ast_decl_t *) top_level_nodes = NULL;

    bool has_token = true;
    while (has_token)
    {
        switch (l.token.type)
        {
        case TOKEN_TYPE_KW_ENUM:
            next_token(&l);
            sb_push(top_level_nodes, parse_enum_decl());
            break;
        case TOKEN_TYPE_KW_STRUCT:
            next_token(&l);
            sb_push(top_level_nodes, parse_aggregate_decl(AST_DECL_STRUCT));
            break;
        case TOKEN_TYPE_KW_UNION:
            next_token(&l);
            sb_push(top_level_nodes, parse_aggregate_decl(AST_DECL_UNION));
            break;
        case TOKEN_TYPE_KW_VAR:
            next_token(&l);
            sb_push(top_level_nodes, parse_const_var_decl(AST_DECL_VAR));
            break;
        case TOKEN_TYPE_KW_CONST:
            next_token(&l);
            sb_push(top_level_nodes, parse_const_var_decl(AST_DECL_CONST));
            break;
        case TOKEN_TYPE_KW_TYPE:
            next_token(&l);
            sb_push(top_level_nodes, parse_type_decl());
            expect_token(';');
            next_token(&l);
            break;
        case TOKEN_TYPE_KW_FN:
            next_token(&l);
            sb_push(top_level_nodes, parse_fn_decl());
            break;
        case TOKEN_TYPE_EOF:
            next_token(&l);
            has_token = false;
            break;
        default:
            printf("Unexpected token\n"); // @Todo Good error handling
            exit(1);
            break;
        }
    }

    return top_level_nodes;
}

void test_parser(void)
{
    init_lexer(&l, "type my_function = fn(i32*, i32[16+7]**): i32;");
    parse_document();
}
