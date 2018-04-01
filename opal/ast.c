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

ast_cmpnd_field_t * ast_new_cmpnd_field(ast_cmpnd_field_type_t type)
{
    ast_cmpnd_field_t * field = xmalloc(sizeof(ast_cmpnd_field_t));
    field->type = type;
    return field;
}

ast_aggregate_item_t * ast_new_aggregate_item()
{
    return xmalloc(sizeof(ast_aggregate_item_t));
}

ast_enum_item_t * ast_new_enum_item()
{
    return xmalloc(sizeof(ast_enum_item_t));
}

