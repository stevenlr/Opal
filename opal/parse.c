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

// operand
// invoke
// un r
// un l
// mult
// add
// cmp
// and
// or
// tern

ast_expr_t * parse_expr(void)
{
    next_token(&l);
    return NULL;
}

ast_typespec_t * parse_typespec(void);
ast_typespec_t * parse_base_type_typespec(void)
{
    ast_typespec_t * typespec = NULL;

    if (is_token(TOKEN_TYPE_IDENTIFIER))
    {
        typespec = xmalloc(sizeof(ast_typespec_t));
        typespec->type          = AST_TYPESPEC_NAME;
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
        typespec = xmalloc(sizeof(ast_typespec_t));
        typespec->type              = AST_TYPESPEC_FN;
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
        ast_typespec_t * parent_type = xmalloc(sizeof(ast_typespec_t));

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

    ast_decl_t * decl = xmalloc(sizeof(ast_decl_t));
    decl->type              = AST_DECL_TYPE;
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

