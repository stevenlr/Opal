#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

void * xmalloc(uint64_t size)
{
    void * ptr = malloc(size);
    assert(ptr);
    return ptr;
}

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

int64_t digit_values[256] =
{
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10, ['A'] = 10,
    ['b'] = 11, ['B'] = 11,
    ['c'] = 12, ['C'] = 12,
    ['d'] = 13, ['D'] = 13,
    ['e'] = 14, ['E'] = 14,
    ['f'] = 15, ['F'] = 15,
};

char escaped_character_chars[256] =
{
    ['0'] = '\0',
    ['n'] = '\n',
    ['t'] = '\t',
    ['r'] = '\r',
    ['b'] = '\b',
    ['\\'] = '\\',
    ['\''] = '\''
};

char escaped_string_chars[256] =
{
    ['0'] = '\0',
    ['n'] = '\n',
    ['t'] = '\t',
    ['r'] = '\r',
    ['b'] = '\b',
    ['\\'] = '\\',
    ['"'] = '"'
};

void next_token(lexer_t * l)
{
    assert(l);

    while (isspace(*l->stream))
    {
        l->stream++;
    }

    switch (*l->stream)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            l->token.type = TOKEN_TYPE_INTEGER;
            l->token.integer = 0;

            int base = 10;
            if (*l->stream == '0')
            {
                base = 8;
                l->stream++;
                switch (*l->stream)
                {
                    case 'x': case 'X':
                        base = 16;
                        l->stream++;
                        break;
                    case 'b': case 'B':
                        base = 2;
                        l->stream++;
                        break;
                    default:
                        assert(isdigit(*l->stream) && "Invalid integer base"); // @Todo A nice error handler 
                }
            }

            while (digit_values[*l->stream] != 0 || *l->stream == '0' || *l->stream == '_')
            {
                if (*l->stream != '_')
                {
                    uint64_t digit_value = digit_values[*l->stream];
                    assert(digit_value < base && "Invalid integer digit");
                    assert(l->token.integer < (UINT64_MAX - digit_value) / base && "Integer overflow");
                    l->token.integer = l->token.integer * base + digit_value;
                }
                l->stream++;
            }
        }
        break;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z':
        case '_':
        {
            l->token.type = TOKEN_TYPE_IDENTIFIER;
            const char * start = l->stream;

            while (isalnum(*l->stream) || *l->stream == '_')
            {
                l->stream++;
            }

            uint64_t length = l->stream - start;
            l->token.identifier = (char *)xmalloc(length + 1);
            memcpy(l->token.identifier, start, length); // @Todo Use intern string
            l->token.identifier[length] = '\0';
        } 
        break;

        case '\'':
        {
            l->token.type = TOKEN_TYPE_CHARACTER; // @Todo Parse \x.. and \0..
            l->stream++;
            assert(*l->stream != '\'' && "Invalid character literal");

            if (*l->stream == '\\')
            {
                *l->stream++;
                assert((escaped_character_chars[*l->stream] != 0 || *l->stream == '\0') && "Invalid escaped character literal");
                l->token.character = escaped_character_chars[*l->stream];
            }
            else
            {
                l->token.character = *l->stream;
            }

            *l->stream++;
            assert(*l->stream == '\'' && "Unexpected character after character literal");
            *l->stream++;
        }
        break;

        case '"':
        {
            l->token.type = TOKEN_TYPE_STRING; // @Todo Parse \x.. and \0..
            l->stream++;
            l->token.string.first = l->stream;

            char * current_write = l->stream;

            while (*l->stream != '"' && *l->stream != '\0')
            {
                if (*l->stream == '\\')
                {
                    l->stream++;
                    assert((escaped_string_chars[*l->stream] != 0 || l->stream == '\0') && "Invalid escaped character in string literal");
                    *current_write = escaped_string_chars[*l->stream];
                }
                else
                {
                    *current_write = *l->stream;
                }

                current_write++;
                l->stream++;
            }

            l->token.string.last = current_write - 1;
            assert(*l->stream == '"' && "End of stream reached inside a string literal");
            l->stream++;
        }
        break;

        default:
        {
            l->token.type = *l->stream++;
        }
        break;
    }
}

void init_lexer(lexer_t * l, char * input)
{
    assert(l);
    assert(input);
    assert(input[strlen(input)] == '\0');

    l->stream = input;
    next_token(l);
}

void test_lexer()
{
    lexer_t lexer;

    {
        init_lexer(&lexer, "hello  +  \t\n12_3 world 7");
        assert(lexer.token.type == TOKEN_TYPE_IDENTIFIER);
        assert(strcmp(lexer.token.identifier, "hello") == 0);

        next_token(&lexer);
        assert(lexer.token.type == '+');

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_INTEGER);
        assert(lexer.token.integer == 123);

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_IDENTIFIER);
        assert(strcmp(lexer.token.identifier, "world") == 0);

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_INTEGER);
        assert(lexer.token.integer == 7);

        next_token(&lexer);
        assert(lexer.token.type == 0);
    }

    {
        init_lexer(&lexer, "0xa_e 015_4 0b0110_0001");
        assert(lexer.token.type == TOKEN_TYPE_INTEGER);
        assert(lexer.token.integer == 174);

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_INTEGER);
        assert(lexer.token.integer == 108);

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_INTEGER);
        assert(lexer.token.integer == 97);

        next_token(&lexer);
        assert(lexer.token.type == 0);
    }

    {
        init_lexer(&lexer, " '@' '\\'' '\\\\' '\\n'");
        assert(lexer.token.type == TOKEN_TYPE_CHARACTER);
        assert(lexer.token.character == '@');

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_CHARACTER);
        assert(lexer.token.character == '\'');

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_CHARACTER);
        assert(lexer.token.character == '\\');

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_CHARACTER);
        assert(lexer.token.character == '\n');

        next_token(&lexer);
        assert(lexer.token.type == 0);
    }

    {
        init_lexer(&lexer, "  \"hello world \\\\ \\\" \n \\n\" \"abc\" ");
        assert(lexer.token.type == TOKEN_TYPE_STRING);
        assert(lexer.token.string.last - lexer.token.string.first + 1 == 19);
        assert(memcmp("hello world \\ \" \n \n", lexer.token.string.first, 19) == 0);

        next_token(&lexer);
        assert(lexer.token.type == TOKEN_TYPE_STRING);
        assert(lexer.token.string.last - lexer.token.string.first + 1 == 3);
        assert(memcmp("abc", lexer.token.string.first, 3) == 0);

        next_token(&lexer);
        assert(lexer.token.type == 0);
    }
}

int main(int argc, char * argv[])
{
    test_lexer();
    return 0;
}