#ifndef _STRVEC_H
#define _STRVEC_H 1

char** new_string_vector(char *sequence, char sep);
void free_string_vector(char **tokens);

#endif // _STRVEC_H
