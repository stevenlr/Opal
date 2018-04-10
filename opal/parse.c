#include "parse.h"
#include "common.h"
#include "lex.h"
#include "ast.h"
#include <stdio.h>
#include <stdbool.h>

lexer_t l;

inline bool is_token(token_type_t type)
{
    return l.token.type == type;
}

inline void expect_token(token_type_t type)
{
    if (!is_token(type))
    {
        printf("Unexpected token %d\n", type);
        exit(1); // @Todo
    }
}

inline bool is_token_cmp_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_CMP_START_ && type < TOKEN_TYPE_CMP_END_;
}

inline bool is_token_add_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_ADD_START_ && type < TOKEN_TYPE_ADD_END_;
}

inline bool is_token_mult_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_MULT_START_ && type < TOKEN_TYPE_MULT_END_;
}

inline bool is_token_invoke_op(void)
{
    token_type_t op = l.token.type;
    return op == TOKEN_TYPE_PARENTHESIS_OPEN 
        || op == TOKEN_TYPE_BRACKET_OPEN
        || op == TOKEN_TYPE_DOT;   
}

inline bool is_token_unary_op(void)
{
    token_type_t op = l.token.type;
    return op == TOKEN_TYPE_PLUS
        || op == TOKEN_TYPE_MINUS
        || op == TOKEN_TYPE_LOGIC_NOT
        || op == TOKEN_TYPE_NOT
        || op == TOKEN_TYPE_AND
        || op == TOKEN_TYPE_MULT;
}

inline bool is_token_assign_op(void)
{
    token_type_t type = l.token.type;
    return type > TOKEN_TYPE_ASSIGN_START_ && type < TOKEN_TYPE_ASSIGN_END_;
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
    ast_decl_t * decl = ast_new_decl(AST_DECL_TYPE);
    decl->name = l.token.identifier;
    next_token(&l);
    expect_token(TOKEN_TYPE_ASSIGN);
    next_token(&l);
    decl->type_decl.type = parse_typespec();
    return decl;
}

ast_simple_stmt_t * parse_simple_stmt(void)
{
    ast_simple_stmt_t * stmt = NULL;

    if (is_token(TOKEN_TYPE_KW_VAR))
    {
        next_token(&l);
        stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_VAR_DECL);
        stmt->var_decl = parse_const_var_decl(AST_DECL_VAR);
    }
    else if (is_token(TOKEN_TYPE_KW_CONST))
    {
        next_token(&l);
        stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_CONST_DECL);
        stmt->const_decl = parse_const_var_decl(AST_DECL_CONST);
    }
    else
    {
        ast_expr_t * expr = parse_expr();

        if (is_token(TOKEN_TYPE_INC))
        {
            next_token(&l);
            stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_INCREMENT);
            stmt->expr = expr;
        }
        else if (is_token(TOKEN_TYPE_DEC))
        {
            next_token(&l);
            stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_DECREMENT);
            stmt->expr = expr;
        }
        else if (is_token_assign_op())
        {
            stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_ASSIGN);
            stmt->assign.op = l.token.type;
            stmt->assign.left = expr;
            next_token(&l);
            stmt->assign.right = parse_expr();
        }
        else
        {
            stmt = ast_new_simple_stmt(AST_SIMPLE_STMT_EXPR);
            stmt->expr = expr;
        }
    }

    if (stmt == NULL)
    {
        printf("Invalid token\n");
        exit(1);
    }
    return stmt;
}

sb_t(ast_simple_stmt_t *) parse_simple_stmt_list(void)
{
    sb_t(ast_simple_stmt_t *) stmts = NULL;
    bool has_stmts = true;
    while (has_stmts)
    {
        ast_simple_stmt_t * stmt = parse_simple_stmt();
        sb_push(stmts, stmt);

        if (!is_token(TOKEN_TYPE_COMMA))
        {
            has_stmts = false;
        }
        else
        {
            next_token(&l);
        }
    }
    return stmts;
}

bool is_switch_case_value_token(void)
{
    return is_token(TOKEN_TYPE_INTEGER)
        || is_token(TOKEN_TYPE_IDENTIFIER);
}

ast_stmt_block_t * parse_stmt_block(void)
{
    expect_token(TOKEN_TYPE_BRACE_OPEN);
    next_token(&l);

    ast_stmt_block_t * block = ast_new_stmt_block();
    block->stmts = NULL;
    block->num_stmts = 0;

    bool has_more_stmts = true;
    while (has_more_stmts)
    {
        ast_stmt_t * stmt = NULL;

        switch (l.token.type)
        {
        case TOKEN_TYPE_KW_IF:
            {
                stmt = ast_new_stmt(AST_STMT_IF);
                stmt->if_stmt.conditions = NULL;
                stmt->if_stmt.stmt_blocks = NULL;
                stmt->if_stmt.num_conditions = 0;
                stmt->if_stmt.else_stmt_block = NULL;

                bool expect_else = false;
                while (is_token(TOKEN_TYPE_KW_IF))
                {
                    next_token(&l);
                    expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
                    next_token(&l);
                    sb_push(stmt->if_stmt.conditions, parse_expr());
                    expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
                    next_token(&l);
                    sb_push(stmt->if_stmt.stmt_blocks, parse_stmt_block());
                    stmt->if_stmt.num_conditions++;

                    if (is_token(TOKEN_TYPE_KW_ELSE))
                    {
                        next_token(&l);
                        if (!is_token(TOKEN_TYPE_KW_IF))
                        {
                            expect_else = true;
                            break;
                        }
                    }
                }

                if (expect_else)
                {
                    stmt->if_stmt.else_stmt_block = parse_stmt_block();
                }
            }
            break;
        case TOKEN_TYPE_KW_WHILE:
            {
                stmt = ast_new_stmt(AST_STMT_WHILE);
                next_token(&l);
                expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
                next_token(&l);
                stmt->while_stmt.condition = parse_expr();
                expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
                next_token(&l);
                stmt->while_stmt.stmt_block = parse_stmt_block();
            }
            break;
        case TOKEN_TYPE_KW_FOR:
            {
                stmt = ast_new_stmt(AST_STMT_FOR);
                next_token(&l);
                expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
                next_token(&l);
                
                stmt->for_stmt.init_stmts = NULL;
                if (!is_token(TOKEN_TYPE_SEMICOLON))
                {
                    stmt->for_stmt.init_stmts = parse_simple_stmt_list();
                }
                stmt->for_stmt.num_init_stmts = sb_len(stmt->for_stmt.init_stmts);
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);

                stmt->for_stmt.condition = parse_expr();
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);

                stmt->for_stmt.incr_stmts = NULL;
                if (!is_token(TOKEN_TYPE_PARENTHESIS_CLOSE))
                {
                    stmt->for_stmt.incr_stmts = parse_simple_stmt_list();
                }
                stmt->for_stmt.num_incr_stmts = sb_len(stmt->for_stmt.incr_stmts);

                expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
                next_token(&l);
                stmt->for_stmt.stmt_block = parse_stmt_block();
            }
            break;
        case TOKEN_TYPE_KW_SWITCH:
            {
                stmt = ast_new_stmt(AST_STMT_SWITCH);
                stmt->switch_stmt.expr = NULL;
                stmt->switch_stmt.items = NULL;
                stmt->switch_stmt.num_items = 0;

                next_token(&l);
                expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
                next_token(&l);
                stmt->switch_stmt.expr = parse_expr();
                expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
                next_token(&l);
                expect_token(TOKEN_TYPE_BRACE_OPEN);
                next_token(&l);

                while (is_switch_case_value_token())
                {
                    ast_switch_item_t * item = ast_new_switch_item();
                    item->values = NULL;
                    item->num_values = 0;
                    item->stmt_block = NULL;

                    while (true)
                    {
                        if (!is_switch_case_value_token())
                        {
                            printf("Invalid case literal token\n");
                            exit(1);
                        }
                        
                        ast_switch_case_literal_t * lit = NULL;
                        switch (l.token.type)
                        {
                        case TOKEN_TYPE_IDENTIFIER:
                            {
                                lit = ast_new_switch_case_literal(AST_CASE_LITERAL_NAME);
                                lit->name = l.token.identifier;
                            }
                            break;
                        case TOKEN_TYPE_INTEGER:
                            {
                                lit = ast_new_switch_case_literal(AST_CASE_LITERAL_INTEGER);
                                lit->integer = l.token.integer;
                            }
                            break;
                        }

                        sb_push(item->values, lit);
                        item->num_values++;
                        next_token(&l);

                        if (!is_token(TOKEN_TYPE_COMMA))
                        {
                            break;
                        }
                        next_token(&l);
                    }

                    expect_token(TOKEN_TYPE_ARROW);
                    next_token(&l);

                    item->stmt_block = parse_stmt_block();
                    sb_push(stmt->switch_stmt.items, item);
                    stmt->switch_stmt.num_items++;
                }

                if (is_token(TOKEN_TYPE_KW_OTHERWISE))
                {
                    next_token(&l);
                    expect_token(TOKEN_TYPE_ARROW);
                    next_token(&l);

                    ast_switch_item_t * item = ast_new_switch_item();
                    item->values = NULL;
                    item->num_values = 0;

                    item->stmt_block = parse_stmt_block();
                    sb_push(stmt->switch_stmt.items, item);
                    stmt->switch_stmt.num_items++;
                }

                expect_token(TOKEN_TYPE_BRACE_CLOSE);
                next_token(&l);
            }
            break;
        case TOKEN_TYPE_KW_RETURN:
            {
                stmt = ast_new_stmt(AST_STMT_RETURN);
                stmt->return_stmt = NULL;
                next_token(&l);
                if (!is_token(TOKEN_TYPE_SEMICOLON))
                {
                    stmt->return_stmt = parse_expr();
                }
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);
            }
            break;
        case TOKEN_TYPE_KW_CONTINUE:
            {
                stmt = ast_new_stmt(AST_STMT_CONTINUE);
                next_token(&l);
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);
            }
            break;
        case TOKEN_TYPE_KW_BREAK:
            {
                stmt = ast_new_stmt(AST_STMT_BREAK);
                next_token(&l);
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);
            }
            break;
        case TOKEN_TYPE_BRACE_OPEN:
            {
                stmt = ast_new_stmt(AST_STMT_BLOCK);
                stmt->stmt_block = parse_stmt_block();
            }
            break;
        case TOKEN_TYPE_BRACE_CLOSE:
            has_more_stmts = false;
            break;
        default:
            {
                stmt = ast_new_stmt(AST_STMT_SIMPLE);
                stmt->simple_stmt = parse_simple_stmt();
                expect_token(TOKEN_TYPE_SEMICOLON);
                next_token(&l);
            }
            break;
        }

        if (stmt != NULL)
        {
            sb_push(block->stmts, stmt);
            block->num_stmts++;
        }
    }

    expect_token(TOKEN_TYPE_BRACE_CLOSE);
    next_token(&l);
    return block;
}

ast_decl_t * parse_fn_decl(void)
{
    ast_decl_t * decl = ast_new_decl(AST_DECL_FN);
    decl->fn_decl.params        = NULL;
    decl->fn_decl.num_params    = 0;
    decl->fn_decl.return_type   = NULL;
    decl->fn_decl.stmt_block    = NULL;

    expect_token(TOKEN_TYPE_IDENTIFIER);
    decl->name = l.token.identifier;
    next_token(&l);
    expect_token(TOKEN_TYPE_PARENTHESIS_OPEN);
    next_token(&l);

    while (!is_token(TOKEN_TYPE_PARENTHESIS_CLOSE))
    {
        ast_param_t * param = ast_new_param();
        expect_token(TOKEN_TYPE_IDENTIFIER);
        param->name = l.token.identifier;
        next_token(&l);
        expect_token(TOKEN_TYPE_COLON);
        next_token(&l);
        param->type = parse_typespec();

        sb_push(decl->fn_decl.params, param);
        decl->fn_decl.num_params++;

        if (!is_token(TOKEN_TYPE_COMMA)) { break; }
        next_token(&l);
    }

    expect_token(TOKEN_TYPE_PARENTHESIS_CLOSE);
    next_token(&l);

    if (is_token(TOKEN_TYPE_COLON))
    {
        next_token(&l);
        decl->fn_decl.return_type = parse_typespec();
    }

    decl->fn_decl.stmt_block = parse_stmt_block();
    return decl;
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
        "enum hello : i32 { hello, popo = cast(f32*[90], 42+59), abab, } enum a : i32 { test = 43, } enum b : i32 { d=9} enum p:i32{d}" 
        "var i : i32; var b: i8 = 45 + 89; const b : i32 = 2;"
        "struct cheese {a: i32; b: i32*[2]*; } union a {a: Vector; } struct popo {}"
        "fn a(a: i32, b: popo) {} fn b(): u64 {"
        "   break; continue; return; return 45 + popo[p + 9](42);"
        "   { return 5; return; {} }"
        "   while (45 + 6) { break; }"
        "   for (; true; ) { break; }"
        "   for (var i: i32 = 0; i < 10; i++) { return i; var i: i32 = 45 + 6; i += test; i++; }"
        "   for (var i: i32 = 45+89, i = 0, const a: i32 = 0x546, i++; true; i++, i += 2, i -= 1) { return i; }"
        "   this_is_a_method_call(a, b, c);"
        "   look_its.oop().chaining[45](45);"
        "   const i: i32*[SIZE] = 45;"
        "   if (true) { if (test) { test(); } "
        "   if (a) {} else { doStuff(); }}"
        "   else if (false) {}"
        "   else if (a+b<c) {}"
        "   if (0) {} else if (1) {} else if (2) {} else {}"
        "   switch (a + 1) {"
        "       '8', 45, MY_VALUE -> { haha(); }"
        "       8 -> {}"
        "       otherwise -> { printf(\"Hello, world\\n\"); }"
        "   }"
        "}"
    );
    parse_document();
}

