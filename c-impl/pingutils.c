#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extractor.h"
#include "pingutils.h"

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


