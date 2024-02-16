#define _DEFAULT_SOURCE

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "strvec.h"
#include "pingutils.h"

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
    // pthread_mutex_destroy(&mutex);
    return 1;
}

void show_help() {
    printf(
        "multiping 0.1.1\n"
        "Ping to multiple host simultaneausly using the ping command installed in your system\n"
        "Usage:\n"
        "  -h, --help: Show this help\n"
        "  -t, --targets TARGETS: the list of targets to ping separated by commas\n"
        "  -c, --counts NUMBER: number of pings\n"
    );
}

int main(int argc, char *argv[]) {
    char *targets = NULL;
    int timeout = 1000;
    int counts = 10;
    const char *short_options = "t:c:o:h";
    const struct option long_options[] = {
        {"targets", required_argument, NULL, 't'},
        {"counts", optional_argument, NULL, 'c'},
        {"timeout", optional_argument, NULL, 'o'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };
    int option;
    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
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
            case 'h':
                show_help();
                exit(EXIT_SUCCESS);
            default:
                show_help();
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


