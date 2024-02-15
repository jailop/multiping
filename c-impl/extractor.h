#ifndef _EXTRACTOR_H
#define _EXTRACTOR_H 1

typedef struct {
    char *target;
    int counts;
    float *length;
    int sent;
    int recv;
    float min;
    float max;
    float avg;
    float stdev;
} ping_t;

int extract_seq(char *log, int *seq, float *lenght);
int extract_packets(char *log, int *sent, int *recv);
int extract_stats(char *log, float *min, float *avg, float *max, float *stdev);

#endif // _EXTRACTOR_H
