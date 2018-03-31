#include "ast.h"
#include "common.h"

ast_decl_t * ast_new_decl(ast_decl_type_t type)
{
    ast_decl_t * decl = xmalloc(sizeof(ast_decl_t));
    decl->type = type;
    return decl;
}

ast_typespec_t * ast_new_typespec(ast_typespec_type_t type)
{
    ast_typespec_t * typespec = xmalloc(sizeof(ast_typespec_t));
    typespec->type = type;
    return typespec;
}

ast_expr_t * ast_new_expr(ast_expr_type_t type)
{
    ast_expr_t * expr = xmalloc(sizeof(ast_expr_t));
    expr->type = type;
    return expr;
}

