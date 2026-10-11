#include <string.h>

#include "parse.h"

int split_tokens(char *line, char *tokens[], int max)
{
    int count = 0;
    char *p = line;

    if (*p == '\0') {
        return 0;
    }

    for (;;) {
        /* token vazio: espaço no início, duplo ou no fim da linha */
        if (*p == ' ' || *p == '\0') {
            return -1;
        }

        if (count >= max) {
            return -1;
        }

        tokens[count++] = p;

        p = strchr(p, ' ');
        if (p == NULL) {
            return count;
        }

        *p = '\0';
        p++;
    }
}