#include <stddef.h>
#include <string.h>
#include <stdlib.h>

char** new_string_vector(char *sequence, char sep) {
    int n = 0;
    char *seq = strdup(sequence);
    for (char *pos = seq; pos != NULL;) {
        n++;
        pos = strchr(pos, sep);
        if (pos) pos++;
    }
    char **tokens = malloc(sizeof(char*) * (n + 1));
    char *pos = seq;
    for (int i = 0; i < n; i++) {
        char *curr = strchr(pos, sep);
        if (NULL != curr) *curr = 0;
        tokens[i] = strdup(pos);
        pos = curr + 1;
    }
    tokens[n] = NULL;
    free(seq);
    return tokens;
}

void free_string_vector(char **tokens) {
    char **curr = tokens;
    while (*curr) {
        free(*curr);
        curr++;
    }
    free(tokens);
}
