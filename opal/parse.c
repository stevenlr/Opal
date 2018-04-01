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

bool is_token_invoke_op(void)
{
    token_type_t op = l.token.type;
    return op == TOKEN_TYPE_PARENTHESIS_OPEN 
        || op == TOKEN_TYPE_BRACKET_OPEN
        || op == TOKEN_TYPE_DOT;   
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

ast_expr_t * parse_expr(void);
ast_typespec_t * parse_typespec(void);

ast_cmpnd_field_t * parse_cmpnd_field(void)
{
    ast_cmpnd_field_t * field = NULL;
    if (is_token(TOKEN_TYPE_DOT) || is_token(TOKEN_TYPE_BRACKET_OPEN))
    {
        if (is_token(TOKEN_TYPE_DOT))
        {
            field = ast_new_cmpnd_field(AST_CMPND_FIELD_FIELD);
            next_token(&l);
            expect_token(TOKEN_TYPE_IDENTIFIER);
            field->field_name = l.token.identifier;
            next_token(&l);
        }
        else
        {
            field = ast_new_cmpnd_field(AST_CMPND_FIELD_INDEX);
            next_token(&l);
            field->index_expr = parse_expr();
            expect_token(TOKEN_TYPE_BRACKET_CLOSE);
            next_token(&l);
        }
        expect_token(TOKEN_TYPE_ASSIGN);
        next_token(&l);
    }
    else
    {
        field = ast_new_cmpnd_field(AST_CMPND_FIELD_EXPR);
    }
    field->expr = parse_expr();
    return field;
}

ast_expr_t * parse_expr_compound(ast_typespec_t * type)
{
    ast_expr_t * expr = ast_new_expr(AST_EXPR_COMPOUND);
    expr->compound.type = type;
    expr->compound.args = NULL;
    expr->compound.num_args = 0;

    expect_token(TOKEN_TYPE_BRACE_OPEN);
    next_token(&l);

    if (!is_token(TOKEN_TYPE_BRACE_CLOSE))
    {
        while (true)
        {
            ast_cmpnd_field_t * field = parse_cmpnd_field();
            sb_push(expr->compound.args, field);
            expr->compound.num_args++;
            if (!is_token(TOKEN_TYPE_COMMA)) { break; }
            next_token(&l);
        }
    }

    expect_token(TOKEN_TYPE_BRACE_CLOSE);
    next_token(&l);
    return expr;
}

ast_expr_t * parse_expr_operand(void)
{
    if (is_token(TOKEN_TYPE_INTEGER))
    {
        ast_expr_t * expr = ast_new_expr(AST_EXPR_INTEGER);
        expr->int_value = l.token.integer;
        next_token(&l);
        return expr;
    }
    else if (is_token(TOKEN_TYPE_IDENTIFIER))
    {
        const char * identifier = l.token.identifier;
        next_token(&l);
        
        if (is_token(TOKEN_TYPE_BRACE_OPEN))
        {
            ast_typespec_t * type = ast_new_typespec(AST_TYPESPEC_NAME);
            type->name = identifier;
            return parse_expr_compound(type);
        }
        
        ast_expr_t * expr = ast_new_expr(AST_EXPR_NAME);
        expr->name = l.token.identifier;
        return expr;
    }
    else if (is_token(TOKEN_TYPE_STRING))
    {
        ast_expr_t * expr = ast_new_expr(AST_EXPR_STRING);
        expr->string_value.str = l.token.string.str;
        expr->string_value.length = l.token.string.length;
        next_token(&l);
        return expr;
    }
    else if (is_token(TOKEN_TYPE_PARENTHESIS_OPEN))
    {
        next_token(&l);
        
        if (is_token(TOKEN_TYPE_COLON))
        {
            next_token(&l);
            ast_typespec_t * type = parse_typespec();
            expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
            next_token(&l);
            ast_expr_t * expr = parse_expr_compound(type);
            return expr;
        }

        ast_expr_t * expr = parse_expr();
        expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
        next_token(&l);
        return expr;
    }
    else if (is_token(TOKEN_TYPE_KW_CAST))
    {
        ast_expr_t * expr = ast_new_expr(AST_EXPR_CAST);
        next_token(&l);
        expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
        next_token(&l);
        expr->cast.type = parse_typespec();
        expect_token(TOKEN_TYPE_COMMA);
        next_token(&l);
        expr->cast.expr = parse_expr();
        expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
        next_token(&l);
        return expr;
    }
    else if (is_token(is_token(TOKEN_TYPE_BRACE_OPEN)))
    {
        return parse_expr_compound(NULL);
    }
    // @Todo handle floats

    printf("Invalid operand\n");
    return NULL;
}

sb_t(ast_expr_t *) parse_expr_list(void)
{
    sb_t(ast_expr_t *) expr_list = NULL;
    while (true)
    {
        ast_expr_t * expr = parse_expr();
        sb_push(expr_list, expr);
        if (!is_token(TOKEN_TYPE_COMMA)) { break; }
        next_token(&l);
    }
    return expr_list;
}

ast_expr_t * parse_expr_invoke(void)
{
    ast_expr_t * expr = parse_expr_operand();
    while (is_token_invoke_op())
    {
        if (is_token(TOKEN_TYPE_PARENTHESIS_OPEN))
        {
            next_token(&l);
            ast_expr_t * args_expr = ast_new_expr(AST_EXPR_INVOKE);
            args_expr->invoke.expr = expr;
            args_expr->invoke.args = NULL;
            args_expr->invoke.num_args = 0;
            expr = args_expr;

            if (!is_token(TOKEN_TYPE_PARENTHESIS_CLOSE))
            {
                sb_t(ast_expr_t *) exprs = parse_expr_list();
                args_expr->invoke.args = exprs;
                args_expr->invoke.num_args = sb_len(exprs);
            }

            expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
            next_token(&l);
        }
        else if (is_token(TOKEN_TYPE_BRACKET_OPEN))
        {
            next_token(&l);
            ast_expr_t * outer = ast_new_expr(AST_EXPR_INDEX);
            outer->index.expr = expr;
            outer->index.index_expr = parse_expr();
            expr = outer;
            expect_token(TOKEN_TYPE_BRACKET_CLOSE);
            next_token(&l);
        }
        else if (is_token(TOKEN_TYPE_DOT))
        {
            next_token(&l);
            expect_token(TOKEN_TYPE_IDENTIFIER);
            ast_expr_t * outer = ast_new_expr(AST_EXPR_FIELD);
            outer->field.expr = expr;
            outer->field.name = l.token.identifier;
            expr = outer;
            next_token(&l);
        }
        else
        {
            printf("Unexpected token\n");
            exit(1);
        }
    }
    
    return expr;
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
    if (is_token(TOKEN_TYPE_QUESTION))
    {
        ast_expr_t * tern_expr = ast_new_expr(AST_EXPR_TERNARY);
        tern_expr->ternary.condition = expr;
        expr = tern_expr;

        next_token(&l);
        tern_expr->ternary.then_expr = parse_expr_tern();
        expect_token(TOKEN_TYPE_COLON);

        next_token(&l);
        tern_expr->ternary.else_expr = parse_expr_tern();
    }
    return expr;
}

ast_expr_t * parse_expr(void)
{
    return parse_expr_tern();
}

ast_typespec_t * parse_base_type_typespec(void)
{
    ast_typespec_t * typespec = NULL;

    if (is_token(TOKEN_TYPE_IDENTIFIER))
    {
        typespec = ast_new_typespec(AST_TYPESPEC_NAME);
        typespec->name          = l.token.identifier;
        next_token(&l);
    }
    else if (is_token(TOKEN_TYPE_PARENTHESIS_OPEN))
    {
        next_token(&l);
        typespec = parse_typespec();
        expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
        next_token(&l);
    }
    else if (is_token(TOKEN_TYPE_KW_FN))
    {
        typespec = ast_new_typespec(AST_TYPESPEC_FN);
        typespec->fn.args           = NULL;
        typespec->fn.num_args       = 0;
        typespec->fn.return_type    = NULL;

        next_token(&l);
        expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
        next_token(&l);

        bool has_args = !is_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
        while (has_args)
        {
            ast_typespec_t * type = parse_typespec();
            sb_push(typespec->fn.args, type);
            typespec->fn.num_args++;

            if (is_token(TOKEN_TYPE_COMMA))
            {
                next_token(&l);
                has_args = true;
            }
            else
            {
                has_args = false;
            }
        }

        expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
        next_token(&l);

        if (is_token(TOKEN_TYPE_COLON))
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

    while (is_token(TOKEN_TYPE_MULT) || is_token(TOKEN_TYPE_BRACKET_OPEN))
    {
        ast_typespec_t * parent_type = ast_new_typespec(AST_TYPESPEC_POINTER);

        if (is_token(TOKEN_TYPE_MULT))
        {
            next_token(&l);
            parent_type->type = AST_TYPESPEC_POINTER;
            parent_type->pointer.base = type;
            type = parent_type;
        }
        else if (is_token(TOKEN_TYPE_BRACKET_OPEN))
        {
            next_token(&l);
            parent_type->type = AST_TYPESPEC_ARRAY;
            parent_type->array.base = type;
            parent_type->array.size_expr = parse_expr();

            expect_token(TOKEN_TYPE_BRACKET_CLOSE);
            next_token(&l);
            type = parent_type;
        }
    }

    return type;
}

ast_decl_t * parse_enum_decl(void)
{
    ast_decl_t * decl = ast_new_decl(AST_DECL_ENUM);
    decl->enum_decl.items = NULL;
    decl->enum_decl.num_items = 0;
    expect_token(TOKEN_TYPE_IDENTIFIER);
    decl->name = l.token.identifier;
    next_token(&l);
    expect_token(TOKEN_TYPE_COLON);
    next_token(&l);
    expect_token(TOKEN_TYPE_IDENTIFIER);
    decl->enum_decl.base_type = parse_typespec();
    expect_token(TOKEN_TYPE_BRACE_OPEN);
    next_token(&l);

    while (true)
    {
        if (is_token(TOKEN_TYPE_BRACE_CLOSE)) { break; }
        expect_token(TOKEN_TYPE_IDENTIFIER);
        ast_enum_item_t * item = ast_new_enum_item();
        item->name = l.token.identifier;
        item->expr = NULL;
        sb_push(decl->enum_decl.items, item);
        decl->enum_decl.num_items++;
        next_token(&l);

        if (is_token(TOKEN_TYPE_ASSIGN))
        {
            next_token(&l);
            item->expr = parse_expr();
        }

        if (!is_token(TOKEN_TYPE_COMMA)) { break; }
        next_token(&l);
    }

    expect_token(TOKEN_TYPE_BRACE_CLOSE);
    next_token(&l);
    return decl;
}

ast_decl_t * parse_aggregate_decl(ast_decl_type_t type)
{
    ast_decl_t * decl = ast_new_decl(type);
    expect_token(TOKEN_TYPE_IDENTIFIER);
    decl->name = l.token.identifier;
    decl->aggregate_decl.items = NULL;
    decl->aggregate_decl.num_items = 0;
    next_token(&l);
    expect_token(TOKEN_TYPE_BRACE_OPEN);
    next_token(&l);

    while (true)
    {
        if (is_token(TOKEN_TYPE_BRACE_CLOSE)) { break; }
        ast_aggregate_item_t * item = ast_new_aggregate_item();
        sb_push(decl->aggregate_decl.items, item);
        decl->aggregate_decl.num_items++;
        expect_token(TOKEN_TYPE_IDENTIFIER);
        item->name = l.token.identifier;
        next_token(&l);
        expect_token(TOKEN_TYPE_COLON);
        next_token(&l);
        item->type = parse_typespec();
        expect_token(TOKEN_TYPE_SEMICOLON);
        next_token(&l);
    }

    expect_token(TOKEN_TYPE_BRACE_CLOSE);
    next_token(&l);
    return decl;
}

ast_decl_t * parse_const_var_decl(ast_decl_type_t type)
{
    ast_decl_t * decl = ast_new_decl(type);
    expect_token(TOKEN_TYPE_IDENTIFIER);
    decl->name = l.token.identifier;
    next_token(&l);
    expect_token(TOKEN_TYPE_COLON);
    next_token(&l);
    decl->var_decl.type = parse_typespec();
    
    if (type == AST_DECL_VAR && !is_token(TOKEN_TYPE_ASSIGN))
    {
        decl->var_decl.expr = NULL;
        return decl;
    }

    expect_token(TOKEN_TYPE_ASSIGN);
    next_token(&l);
    decl->var_decl.expr = parse_expr();
    return decl;
}

ast_decl_t * parse_type_decl(void)
{
    expect_token(TOKEN_TYPE_IDENTIFIER);
    const char * name = l.token.identifier;

    next_token(&l);
    expect_token(TOKEN_TYPE_ASSIGN);
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
            expect_token(TOKEN_TYPE_SEMICOLON);
            next_token(&l);
            break;
        case TOKEN_TYPE_KW_CONST:
            next_token(&l);
            sb_push(top_level_nodes, parse_const_var_decl(AST_DECL_CONST));
            expect_token(TOKEN_TYPE_SEMICOLON);
            next_token(&l);
            break;
        case TOKEN_TYPE_KW_TYPE:
            next_token(&l);
            sb_push(top_level_nodes, parse_type_decl());
            expect_token(TOKEN_TYPE_SEMICOLON);
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
    init_lexer(&l,
        "type my_function = fn(i32*, i32[16+7 + (:Vector[2]*){4+5, 78} + Vector{ .cheese = 42+42, ['5'] = 5 }]**): i32;"
        "enum hello : i32 { hello, popo = 42+59, abab, } enum a : i32 { test = 43, } enum b : i32 { d=9} enum p:i32{d}" 
        "var i : i32; var b: i8 = 45 + 89; const b : i32 = 2;"
        "struct cheese {a: i32; b: i32*[2]*; } union a {a: Vector; } struct popo {}"
    );
    parse_document();
}

