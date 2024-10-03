#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdio.h>
#include <string.h>
#include "main.h"

typedef struct {
    uint32_t	* const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

#define CIRC_BBUF_DEF(x,y)                \
    uint32_t x##_data_space[y];            \
    circ_bbuf_t x = {                     \
        .buffer = x##_data_space,         \
        .head = 0,                        \
        .tail = 0,                        \
        .maxlen = y                       \
    }

int circ_bbuf_push(circ_bbuf_t *c, uint32_t data);
int circ_bbuf_pop(circ_bbuf_t *c, uint32_t *data);


#endif
