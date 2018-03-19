#pragma once

#include <stdint.h>

// @Todo Multi-characters operators
// @Todo Keywords
// @Todo Floating point numbers
typedef enum token_type_t
{
    TOKEN_TYPE_EOF = 0,
    TOKEN_TYPE_LAST_ASCII = 127,
    TOKEN_TYPE_INTEGER,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_CHARACTER,
    TOKEN_TYPE_STRING
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
