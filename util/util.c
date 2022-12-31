#include "../include/util.h"

void memset(void *s, int val, size_t len) {
    char *p = s;
    for (int i = 0; i < len; i++)
        p[i] = val;
}
