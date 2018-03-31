#pragma once

#include "lex.h"

typedef struct ast_expr_t;
typedef struct ast_stmt_block_t;

typedef enum ast_typespec_type_t
{
    AST_TYPESPEC_NAME,
    AST_TYPESPEC_ARRAY,
    AST_TYPESPEC_POINTER,
    AST_TYPESPEC_FN
} ast_typespec_type_t;

typedef struct ast_typespec_t
{
    ast_typespec_type_t type;
    union
    {
        const char * name;
        struct
        {
            struct ast_typespec_t * base;
            struct ast_expr_t * size_expr;
        } array;
        struct
        {
            struct ast_typespec_t * base;
        } pointer;
        struct
        {
            sb_t(struct ast_typespec_t *) args;
            int32_t num_args;
            struct ast_typespec_t * return_type;
        } fn;
    };
} ast_typespec_t;

typedef enum ast_expr_type_t
{
    AST_EXPR_TERNARY,
    AST_EXPR_BINARY_OP,
    AST_EXPR_UNARY_OP,
    AST_EXPR_CAST,
    AST_EXPR_INVOKE,
    AST_EXPR_INDEX,
    AST_EXPR_FIELD,
    AST_EXPR_COMPOUND,
    AST_EXPR_NAME,
    AST_EXPR_STRING,
    AST_EXPR_INTEGER,
    AST_EXPR_FLOAT
} ast_expr_type_t;

typedef struct ast_expr_t
{
    ast_expr_type_t type;
    union
    {
        struct
        {
            struct ast_expr_t * condition;
            struct ast_expr_t * then_expr;
            struct ast_expr_t * else_expr;
        } ternary;
        struct
        {
            token_type_t op;
            struct st_expr_t * left;
            struct st_expr_t * right;
        } binary;
        struct
        {
            token_type_t op;
            struct ast_expr_t * expr;
        } unary;
        struct
        {
            ast_typespec_t * type;
            struct ast_expr_t * expr;
        } cast;
        struct
        {
            struct ast_expr_t * expr;
            sb_t(struct ast_expr_t *) args;
            int32_t num_args;
        } invoke;
        struct
        {
            struct ast_expr_t * expr;
            struct ast_expr_t * index_expr;
        } index;
        struct
        {
            struct ast_expr_t * expr;
            const char * name;
        } field;
        struct
        {
            ast_typespec_t * type;
            sb_t(struct ast_expr_t *) args;
            int32_t num_args;
        } compound;
        const char * name;
        struct
        {
            sb_t(const char) str;
            uint64_t length;
        } string_value;
        int64_t int_value;
        double float_value;
    };
} ast_expr_t;

typedef enum ast_simple_stmt_type_t
{
    AST_SIMPLE_STMT_VAR_DECL,
    AST_SIMPLE_STMT_CONST_DECL,
    AST_SIMPLE_STMT_ASSIGN,
    AST_SIMPLE_STMT_DECREMENT,
    AST_SIMPLE_STMT_INCREMENT,
    AST_SIMPLE_STMT_EXPR
} ast_simple_stmt_type_t;

typedef struct ast_simple_stmt_t
{
    ast_simple_stmt_type_t type;
    union
    {
        ast_expr_t * expr;
        struct
        {
            ast_expr_t * expr;
        } inc, dec;
        struct
        {
            token_type_t op;
            ast_expr_t * right;
            ast_expr_t * left;
        } assign;
        struct
        {
            const char * name;
            ast_typespec_t * type;
            ast_expr_t * expr;
        } var_decl, const_decl;
    };
} ast_simple_stmt_t;

typedef enum ast_stmt_type_t
{
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_FOR,
    AST_STMT_SWITCH,
    AST_STMT_RETURN,
    AST_STMT_CONTINUE,
    AST_STMT_BREAK,
    AST_STMT_BLOCK,
    AST_STMT_SIMPLE
} ast_stmt_type_t;

typedef enum ast_switch_case_literal_type_t
{
    AST_CASE_LITERAL_NAME,
    AST_CASE_LITERAL_INTEGER
} ast_switch_case_literal_type_t;

typedef struct ast_switch_case_literal_t
{
    ast_switch_case_literal_type_t type;
    union
    {
        const char * name;
        int64_t integer;
    };
} ast_switch_case_literal_t;

typedef struct ast_switch_item_t
{
    sb_t(ast_switch_case_literal_t *) values;
    int32_t num_values;
    bool is_otherwise;
    struct ast_stmt_block_t * stmt_block;
} ast_switch_item_t;

typedef struct ast_stmt_t
{
    ast_stmt_type_t type;
    union
    {
        struct
        {
            sb_t(ast_expr_t *) conditions;
            struct ast_stmt_block_t * stmt_blocks;
            int32_t num_conditions;
            struct ast_stmt_block_t * else_stmt_block;
        } if_stmt;
        struct
        {
            ast_expr_t * condition;
            struct ast_stmt_block_t * stmt_block;
        } while_stmt;
        struct
        {
            sb_t(ast_simple_stmt_t *) init_stmts;
            int32_t num_init_stmts;
            ast_expr_t * condition;
            sb_t(ast_simple_stmt_t *) incr_stmts;
            int32_t num_incr_stmts;
        } for_stmt;
        struct
        {
            ast_expr_t * expr;
            sb_t(ast_switch_item_t *) items;
            int32_t num_items;
        } switch_stmt;
        ast_expr_t * return_stmt;
        ast_simple_stmt_t * simple_stmt;
        struct ast_stmt_block_t * stmt_block;
    };
} ast_stmt_t;

typedef struct ast_stmt_block_t
{
    sb_t(ast_stmt_t *) stmts;
    int32_t num_stmts;
} ast_stmt_block_t;

typedef enum ast_decl_type_t
{
    AST_DECL_ENUM,
    AST_DECL_UNION,
    AST_DECL_STRUCT,
    AST_DECL_VAR,
    AST_DECL_CONST,
    AST_DECL_TYPE,
    AST_DECL_FN
} ast_decl_type_t;

typedef struct ast_enum_item_t
{
    const char * name;
    ast_typespec_t * expr;
} ast_enum_item_t;

typedef struct ast_aggregate_item_t
{
    const char * name;
    ast_typespec_t * type;
    ast_expr_t * expr;
} ast_aggregate_item_t;

typedef struct ast_param_t
{
    const char * name;
    ast_typespec_t * type;
} ast_param_t;

typedef struct ast_decl_t
{
    ast_decl_type_t type;
    const char *name;
    union
    {
        struct
        {
            ast_typespec_t * type;
        } type_decl;
        struct
        {
            ast_typespec_t * type;
            ast_expr_t * expr;
        } var_decl, const_decl;
        struct
        {
            ast_typespec_t * base_type;
            sb_t(ast_enum_item_t *) items;
            int32_t num_items;
        } enum_decl;
        struct
        {
            sb_t(ast_aggregate_item_t *) items;
            int32_t num_items;
        } aggregate_decl;
        struct
        {
            sb_t(ast_param_t *) params;
            int32_t num_params;
            ast_typespec_t * return_type;
            ast_stmt_block_t * stmt_block;
        } fn_decl;
    };
} ast_decl_t;

ast_decl_t * ast_new_decl(ast_decl_type_t type);
ast_typespec_t * ast_new_typespec(ast_typespec_type_t type);
ast_expr_t * ast_new_expr(ast_expr_type_t type);

