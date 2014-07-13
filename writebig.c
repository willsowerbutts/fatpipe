#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "libcpm.h"
#include "crc32.h"

unsigned char buffer[CPM_BLOCK_SIZE];

void main(int argc, char *argv[])
{
    int i, r;
    unsigned int block;
    cpm_fcb fcb;

    for(i=0; i<CPM_BLOCK_SIZE; i++)
        buffer[i] = (unsigned char)i;

    if(argc < 2){
        printf("Please specify test filename.\n");
        cpm_abort();
    }

    cpm_f_prepare(&fcb, argv[1]);
    cpm_f_delete(&fcb);
    cpm_f_create(&fcb);
    block=0;
    for(i=0; i<16384; i++){
        printf("\r%d ", i);
        r = cpm_f_write_random(&fcb, block, buffer);
        if(r != 0){
            printf("write block %d: r=%d\n", block, r);
        }
        block++;
    }
    cpm_f_close(&fcb);
    printf("\n");
}
