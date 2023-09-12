#ifndef LOG_H
#define LOG_H
#include <stdio.h>

#define ERROR(...)\
    {\
    fprintf(stderr, "%s:%d\t", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    }

#endif//LOG_H