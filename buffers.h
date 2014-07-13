#ifndef __BUFFERS_DOT_H__
#define __BUFFERS_DOT_H__

#include "libcpm.h"

/* storage for our buffers is defined in buffers.s so we can force them into the _BSS section */

#define CPM_BLOCKS_PER_MESSAGE (64)

typedef struct {
    unsigned int magic;
} fileblock_msg_header_t;

typedef struct {
    fileblock_msg_header_t header;
    char data[CPM_BLOCKS_PER_MESSAGE * CPM_BLOCK_SIZE];
} fileblock_msg_t;

extern fileblock_msg_t buffer_a, buffer_b;

#endif
