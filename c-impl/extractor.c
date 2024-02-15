#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <math.h>

int extract_seq(char *log, int *seq, float *length) {
    regex_t regex;
    regmatch_t matches[3];
    *seq = -1;
    *length = nanf("1");
    const char *pattern = "icmp_seq=([0-9]+) .* time=([0-9.]+) ms";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        regfree(&regex);
        return -1;
    }
    if (regexec(&regex, log, 3, matches, 0) != 0) {
        regfree(&regex);
        return -1;
    }
    sscanf(log + matches[1].rm_so, "%d", seq);
    sscanf(log + matches[2].rm_so, "%f", length);
    return 0;
}

int extract_packets(char *log, int *sent, int *recv) {
    regex_t regex;
    regmatch_t matches[3];
    *sent = -1;
    *recv = -1;
    const char *pattern = "([0-9]+) packets transmitted, ([0-9]+) received";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        regfree(&regex);
        return -1;
    }
    if (regexec(&regex, log, 3, matches, 0) != 0) {
        regfree(&regex);
        return -1;
    }
    sscanf(log + matches[1].rm_so, "%d", sent);
    sscanf(log + matches[2].rm_so, "%d", recv);
    return 0;
}

int extract_stats(char *log, float *min, float *avg, float *max, float *stdev) {
    regex_t regex;
    regmatch_t matches[5];
    *min = *max = *avg = *stdev = -1;
    const char *pattern = "rtt min/avg/max/mdev = "
        "([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        regfree(&regex);
        return -1;
    }
    if (regexec(&regex, log, 5, matches, 0) != 0) {
        regfree(&regex);
        return -1;
    }
    sscanf(log + matches[1].rm_so, "%f", min);
    sscanf(log + matches[2].rm_so, "%f", avg);
    sscanf(log + matches[3].rm_so, "%f", max);
    sscanf(log + matches[4].rm_so, "%f", stdev);
    return 0;
}
