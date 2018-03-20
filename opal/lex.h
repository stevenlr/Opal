#pragma once

#include <stdint.h>

// @Todo Keywords
// @Todo Floating point numbers
typedef enum token_type_t
{
    TOKEN_TYPE_EOF = 0,
    TOKEN_TYPE_LAST_ASCII = 127,

    // Literals
    TOKEN_TYPE_INTEGER,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_CHARACTER,
    TOKEN_TYPE_STRING,

    // Operators
    TOKEN_TYPE_LE,
    TOKEN_TYPE_GE,
    TOKEN_TYPE_NE,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_LOGIC_AND,
    TOKEN_TYPE_LOGIC_OR,
    TOKEN_TYPE_INC,
    TOKEN_TYPE_DEC,
    TOKEN_TYPE_SHR,
    TOKEN_TYPE_SHL,

    // Assignments
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

    // Keywords
    //TOKEN_TYPE_KW_VAR,
    //TOKEN_TYPE_KW_CONST,
    //TOKEN_TYPE_KW_LET,
    //TOKEN_TYPE_KW_IF,
    //TOKEN_TYPE_KW_ELSE,
    //TOKEN_TYPE_KW_WHILE,
    //TOKEN_TYPE_KW_FOR,
    //TOKEN_TYPE_KW_SWITCH,
    //TOKEN_TYPE_KW_OTHERWISE,
    //TOKEN_TYPE_KW_RETURN,
    //TOKEN_TYPE_KW_FN,
    //TOKEN_TYPE_KW_STRUCT,
    //TOKEN_TYPE_KW_UNION,
    //TOKEN_TYPE_KW_ENUM,
    //TOKEN_TYPE_KW_TYPE,
    
    END
} token_type_t;

typedef struct token_t
{
    token_type_t type;
    union
    {
        uint64_t integer;
        char * identifier;
        char character;
        struct
        {
            char * first;
            char * last;
        } string;
    };
} token_t;

typedef struct lexer_t
{
    char * stream;
    token_t token;
} lexer_t;

void next_token(lexer_t * l);
void init_lexer(lexer_t * l, char * input);
void test_lexer();
