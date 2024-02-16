#ifndef _PINGUTILS_H
#define _PINGUTILS_H 1

#define STRLEN 256
#define QUEUELEN 100

typedef struct {
    char target[STRLEN];
    char content[STRLEN];
} message_t;

typedef struct {
    char *target;
    int timeout;
    int counts;
} task_t;

void *receive_and_print(void *data);
void *exec_ping(void *data);

#endif // _PINGUTILS_H
