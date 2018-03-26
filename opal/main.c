#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "lex.h"
#include "parse.h"

int main(int argc, char * argv[])
{
    test_common();
    test_lexer();
    test_parser();
    return 0;
}
