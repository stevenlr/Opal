#pragma once

#include <stdint.h>
#include "common.h"

// @Todo Floating point numbers
typedef enum token_type_t
{
    TOKEN_TYPE_EOF = 0,

    // Literals
    TOKEN_TYPE_INTEGER,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_STRING,

    // Operators
    TOKEN_TYPE_CMP_START_,
    TOKEN_TYPE_GT,
    TOKEN_TYPE_LT,
    TOKEN_TYPE_LE,
    TOKEN_TYPE_GE,
    TOKEN_TYPE_NE,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_CMP_END_,

    TOKEN_TYPE_ADD_START_,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_OR,
    TOKEN_TYPE_XOR,
    TOKEN_TYPE_ADD_END_,

    TOKEN_TYPE_MULT_START_,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_MULT,
    TOKEN_TYPE_DIV,
    TOKEN_TYPE_MOD,
    TOKEN_TYPE_SHR,
    TOKEN_TYPE_SHL,
    TOKEN_TYPE_MULT_END_,

    TOKEN_TYPE_NOT,

    TOKEN_TYPE_LOGIC_NOT,
    TOKEN_TYPE_LOGIC_AND,
    TOKEN_TYPE_LOGIC_OR,
    TOKEN_TYPE_INC,
    TOKEN_TYPE_DEC,

    // Assignments
    TOKEN_TYPE_ASSIGN,
    TOKEN_TYPE_ASSIGN_ADD,
    TOKEN_TYPE_ASSIGN_SUB,
    TOKEN_TYPE_ASSIGN_MULT,
    TOKEN_TYPE_ASSIGN_DIV,
    TOKEN_TYPE_ASSIGN_MOD,
    TOKEN_TYPE_ASSIGN_NOT,
    TOKEN_TYPE_ASSIGN_AND,
    TOKEN_TYPE_ASSIGN_OR,
    TOKEN_TYPE_ASSIGN_XOR,
    TOKEN_TYPE_ASSIGN_SHR,
    TOKEN_TYPE_ASSIGN_SHL,

    // Punctuation
    TOKEN_TYPE_ARROW,
    TOKEN_TYPE_QUESTION,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_BRACKET_OPEN,
    TOKEN_TYPE_BRACKET_CLOSE,
    TOKEN_TYPE_BRACE_OPEN,
    TOKEN_TYPE_BRACE_CLOSE,
    TOKEN_TYPE_PARENTHESIS_OPEN,
    TOKEN_TYPE_PARENTHESIS_CLOSE,

    // Keywords
    TOKEN_TYPE_KW_START_,
    TOKEN_TYPE_KW_VAR,
    TOKEN_TYPE_KW_CONST,
    TOKEN_TYPE_KW_IF,
    TOKEN_TYPE_KW_ELSE,
    TOKEN_TYPE_KW_WHILE,
    TOKEN_TYPE_KW_FOR,
    TOKEN_TYPE_KW_SWITCH,
    TOKEN_TYPE_KW_OTHERWISE,
    TOKEN_TYPE_KW_RETURN,
    TOKEN_TYPE_KW_FN,
    TOKEN_TYPE_KW_STRUCT,
    TOKEN_TYPE_KW_UNION,
    TOKEN_TYPE_KW_ENUM,
    TOKEN_TYPE_KW_CAST,
    TOKEN_TYPE_KW_TYPE,
    TOKEN_TYPE_KW_CONTINUE,
    TOKEN_TYPE_KW_BREAK,
    TOKEN_TYPE_KW_END_,
} token_type_t;

typedef struct token_t
{
    token_type_t type;
    union
    {
        uint64_t integer;
        const char * identifier;
        char character;
        struct
        {
            sb_t(const char) str;
            uint64_t length;
        } string;
    };
} token_t;

typedef struct lexer_t
{
    const char * stream;
    token_t token;
} lexer_t;

void next_token(lexer_t * l);
void init_lexer(lexer_t * l, const char * input);
void test_lexer(void);
