#define _DEFAULT_SOURCE

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "strvec.h"
#include "extractor.h"

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

pthread_mutex_t mutex;
message_t message_queue[QUEUELEN];
int queue_pos = 0;

void enqueue_message(char *target, char *message) {
    if (NULL == target) return;
    pthread_mutex_lock(&mutex);
    if (queue_pos < QUEUELEN) {
        strncpy(message_queue[queue_pos].target, target, STRLEN);
        strncpy(message_queue[queue_pos++].content, message, STRLEN);
    }
    pthread_mutex_unlock(&mutex);
}

void *receive_and_print(void *data) {
    while(1) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < queue_pos; i++)
            printf("%s: %s", message_queue[i].target, message_queue[i].content);
        queue_pos = 0;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void ping_message(ping_t *pingres, char *message) {
    int seq;
    float length;
    if (extract_seq(message, &seq, &length) == 0) {
        sprintf(message, "seq %d time %.3f ms\n", seq, length);
        return;
    }
    if (extract_packets(message, &(pingres->sent), &(pingres->recv)) == 0) {
        float loss = 100.0 - pingres->recv * 100.0 / pingres->sent;
        sprintf(message, "sent: %d received: %d loss: %0.1f%c\n",
            pingres->sent, pingres->recv, loss, '%');
        enqueue_message(pingres->target, message);
    }
    else if (extract_stats(message, &(pingres->min), &(pingres->avg), &(pingres->max), &(pingres->stdev)) == 0) {
        sprintf(message, "min: %.3f avg: %.3f max: %.3f stdev: %0.3f (ms)\n",
            pingres->min, pingres->avg, pingres->max, pingres->stdev);
        enqueue_message(pingres->target, message);
    }
    else
        enqueue_message(NULL, NULL);
}

void *exec_ping(void *data) {
    task_t *task = (task_t *) data;
    char cmd[STRLEN];
    sprintf(cmd, "ping -c %d %s", task->counts, task->target);
    FILE *pipe = popen(cmd, "r");
    if (pipe != NULL) {
        char message[STRLEN];
        ping_t pingres;
        pingres.target = task->target;
        while (fgets(message, STRLEN, pipe) != NULL) {
            ping_message(&pingres, message); 
        }
        pclose(pipe);
    }
    else {
        perror("Error openning pipe");
    }
    free(task);
    sleep(3);
    pthread_exit(NULL);
}

int launch_workers(char *targets[], int n_targets, int timeout, int counts) {
    pthread_t receiver;
    if (pthread_create(&receiver, NULL, receive_and_print, NULL) != 0) {
        fprintf(stderr, "Error trying to create a new receiver thread");
        exit(EXIT_FAILURE);
    }
    pthread_t threads[n_targets];
    for (int i = 0; i < n_targets; i++) {
        task_t *task = malloc(sizeof(task));
        task->target = targets[i];
        task->timeout = timeout;
        task->counts = counts;
        if (pthread_create(&threads[i], NULL, exec_ping, task) != 0) {
            fprintf(stderr, "Error trying to create a new thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < n_targets; i++) {
        if (pthread_join(threads[i], NULL) != 0)
            fprintf(stderr, "Error joining thread %d\n", i);
    }
    sleep(1);
    pthread_cancel(receiver);
    pthread_mutex_destroy(&mutex);
    return 1;
}

int main(int argc, char *argv[]) {
    const char *shortOptions = "t:c:o:";  // Short options for getopt
    const struct option longOptions[] = {
        {"targets", required_argument, NULL, 't'},
        {"counts", optional_argument, NULL, 'c'},
        {"timeout", optional_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}  // End of options
    };
    char *targets = NULL;
    int timeout = 1000;
    int counts = 10;
    int option;
    while ((option = getopt_long(argc, argv, shortOptions, longOptions, NULL)) != -1) {
        switch (option) {
            case 't':
                targets = optarg;
                break;
            case 'c':
                counts = optarg ? atoi(optarg) : 10;
                break;
            case 'o':
                timeout = optarg ? atoi(optarg) : 1000;
                break;
            default:
                fprintf(stderr, "Usage: %s --target LIST_OF_TARGETS [--counts NUMBER] [--timeout NUMBER]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (NULL == targets) {
        fprintf(stderr, "Error: --targets must be provided\n");
        exit(EXIT_FAILURE);
    }
    char **tokens = new_string_vector(targets, ',');
    int n_tokens = 0;
    for (char **curr = tokens; NULL != *curr; curr++) n_tokens++;
    return !launch_workers(tokens, n_tokens, timeout, counts);
}
