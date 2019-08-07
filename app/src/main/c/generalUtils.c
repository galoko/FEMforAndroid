#include "generalUtils.h"

#include <time.h>

#define BILLION 1E9

double getTime(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + time.tv_nsec / BILLION;
}

#define MILLION 1E6

int64_t timeToUSec(double time) {
    return (int64_t)(time * MILLION);
}