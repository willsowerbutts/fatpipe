#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "libcpm.h"
#include "crc32.h"

unsigned char buffer[CPM_BLOCK_SIZE];

void main(int argc, char *argv[])
{
    int i, r;
    bool eof;
    unsigned long crc;
    unsigned int block;
    cpm_fcb fcb;

    for(i=1; i<argc; i++){
        cpm_f_prepare(&fcb, argv[i]);
        printf("%-12s ", argv[i]);

        if(cpm_f_open(&fcb)){
            printf("Cannot open\n");
            continue;
        }

        crc=0xFFFFFFFFUL;
        block=0;
        eof=false;
        while(!eof){
            r = cpm_f_read_random(&fcb, block, buffer);
            switch(r){
                case 1:
                case 4:
                    eof=true;
                    break; 
                case 0:
                    break;
                default:
                    printf("read failed :( ");
                    eof=true;
            }
            if(!eof){
                crc = crc32(buffer, CPM_BLOCK_SIZE, crc);
            }
            block++;
        }

        crc = ~crc;
        printf("%08lx\n", crc);
    }
}
