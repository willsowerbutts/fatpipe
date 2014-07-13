#define Z180_IO_BASE (0x40)
#include <z180/z180.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "libcpm.h"
#include "crc32.h"
#include "buffers.h"

/* additional Z8S180 ASCI registers */
__sfr __at (Z180_IO_BASE+0x12) ASEXT0;  /* ASCI extension register 0 */
__sfr __at (Z180_IO_BASE+0x13) ASEXT1;  /* ASCI extension register 1 */
__sfr __at (Z180_IO_BASE+0x1A) ASTC0L;  /* ASCI Time constant register 0 Low */
__sfr __at (Z180_IO_BASE+0x1B) ASTC0H;  /* ASCI Time constant register 0 High */
__sfr __at (Z180_IO_BASE+0x1C) ASTC1L;  /* ASCI Time constant register 1 Low */
__sfr __at (Z180_IO_BASE+0x1D) ASTC1H;  /* ASCI Time constant register 1 High */
__sfr __at (Z180_IO_BASE+0x2D) IAR1B ;  /* DMA I/O address reg,    channel 1B */

#define PHI  (18432000*2)   /* CPU frequency */
#define BAUD 576000         /* desired baud rate */

void reset_dma(void);

void dump_mem(void *addr, unsigned int length)
{
    unsigned char *p=(unsigned char*)addr;

    while(length--){
        printf("%02x ", *(p++));
    }

    printf("\n");
}

void dump_regs(void)
{
    printf("ASCI0: CNTLA0=%02x CNTLB0=%02x STAT0=%02x ASTC0H=%02x ASTC0L=%02x ASEXT0=%02x\n",
            CNTLA0, CNTLB0, STAT0, ASTC0H, ASTC0L, ASEXT0);
    printf("ASCI1: CNTLA1=%02x CNTLB1=%02x STAT1=%02x ASTC1H=%02x ASTC1L=%02x ASEXT1=%02x\n",
            CNTLA1, CNTLB1, STAT1, ASTC1H, ASTC1L, ASEXT1);
    printf("DMA: BCR1=%02x%02x MAR1=%02x%02x%02x IAR1=%02x%02x DSTAT=%02x DMODE=%02x DCNTL=%02x\n",
            BCR1H, BCR1L, MAR1B, MAR1H, MAR1L, IAR1H, IAR1L, DSTAT, DMODE, DCNTL);
            
}

void die(void)
{
    /* it all went wrong */
    dump_regs();
    reset_dma();
    cpm_abort();
}

#define SPINNER_LENGTH 4
static char spinner_char[SPINNER_LENGTH] = {'|', '/', '-', '\\'};
static unsigned char spinner_pos=0;

char spinner(void)
{
    spinner_pos = (spinner_pos + 1) % SPINNER_LENGTH;
    return spinner_char[spinner_pos];
}

/* determine the physical address corresponding with a virtual address in the current MMU mapping */
unsigned long virtual_to_physical(void *_vaddr)
{
    unsigned int vaddr=(unsigned int)_vaddr;

    /* memory is arranged as (low->high) common0, banked, common1 */
    /* physical base of common0 is always 0. banked, common1 base specified by BBR, CBR */
    /* virtual start of banked, common1 specified by CBAR */

    if(vaddr < ((unsigned int)(CBAR & 0x0F)) << 12){ /* if address is before banked base, it's in common0 */
        return vaddr; /* unmapped */
    }

    if(vaddr < ((unsigned int)(CBAR & 0xF0)) << 8){ /* if before common1 base, it's in banked */
        return ((unsigned long)BBR << 12) + vaddr;
    }

    /* it's in common1 */
    return ((unsigned long)CBR << 12) + vaddr;
}

void configure_uart(void)
{
    unsigned int tc;

    /* compute time constant value */
    tc = (PHI / (2 * BAUD * 16)) - 2;       /* 16X divisor */

    /* configure ASCI1 */
    CNTLA1 = 0x00;    /* disable UART */
    CNTLB1 = 0x00;
    STAT1  = 0x00;
    ASTC1L = tc & 0xff;
    ASTC1H = tc >> 8;
    ASEXT1 = 0x08;    /* 16X divisor -- cannot use 1X, requires RXA synchronised to CKA :(    */
    CNTLA1 = 0x64;    /* enable UART, 8N1                                                     */

}

void uart_transmit(void *ptr, unsigned int length)
{
    char *p = (char*)ptr;

    while(length--){
        while(!(STAT1 & 0x02)); /* wait for transmit data register to be empty */
        TDR1 = *(p++);
    }
}

void uart_receive(void *ptr, unsigned int length)
{
    char *p = (char*)ptr;
    unsigned int to;
    unsigned char t;
    while(length--){
        to = 0x1000;
        t = 0;
        while(!(STAT1 & 0x80)){ /* wait for receive data register to be full */
            if(!++t){
                if(!to--){
                    printf("Receive timeout (waiting for %d bytes)\n", length+1);
                    die();
                }
            }
        }
        *(p++) = RDR1;
    }
}

void prepare_dma_receive(void *ptr, unsigned int length)
{
    /* program DMA engine to receive data directly into memory */
    unsigned long physaddr = virtual_to_physical(ptr);
    volatile unsigned char junk;

    DSTAT = DSTAT & 0x5F; /* disable DMA1 */

    /* memory address */
    MAR1L = physaddr;
    MAR1H = physaddr >> 8;
    MAR1B = (physaddr >> 16) & 0x0F;

    /* I/O address */
    IAR1L = 0x49; /* ASCI1 receive register address */
    IAR1H = 0x00;
    IAR1B = 0x02; /* ASCI1 RDRF */

    /* byte count */
    BCR1L = length;
    BCR1H = length >> 8;

    DCNTL = (DCNTL & 0xF4) | (0x08 | 0x02); /* edge sensitive, I/O -> memory, MAR1 increments */

    DSTAT = 0x80 | (DSTAT & 0x5F); /* enable DMA1 */

    /* data sheet says the RDRF bit must be zero or DMA will never start */
    while(STAT1 & 0x80){
        junk = RDR1; /* flush the receive FIFO */
    }
}

void reset_dma(void)
{
    DSTAT = DSTAT & 0x5F; /* disable DMA1 */
}

void wait_receive_complete(void)
{
    unsigned long timeout = 0x1ffff;
    while(DSTAT & 0x80){
        if(!(timeout--)){
            printf("DMA receive timeout BCR1=0x%02x%02x\n", BCR1H, BCR1L);
            die();
        }
    }
}

#define MSG_MAGIC (0x16A5)
#define MSG_CODE_HELLO     1
#define MSG_CODE_FILEINFO  2
#define MSG_CODE_SENDBLOCK 3
#define MSG_CODE_SENDCRC   4
#define MSG_CODE_FILEDONE  5

typedef struct {
    unsigned int magic;
    unsigned int data0;
    unsigned int data1;
    unsigned int data2;
} message_t;

typedef struct {
    unsigned int magic;
    char filename[16];
    unsigned int blocks;
    /* This is the first thing we send so the first opportunity to detect
     * corruption on the link. We use a CRC of the message (excluding the
     * CRC bytes) */
    unsigned long msg_crc;
} fileinfo_msg_t;

typedef struct {
    unsigned int magic;
    unsigned long crc;
} filecrc_msg_t;

void send_message(unsigned int d0, unsigned int d1, unsigned int d2)
{
    message_t msg;

    msg.magic = MSG_MAGIC;
    msg.data0 = d0;
    msg.data1 = d1;
    msg.data2 = d2;

    uart_transmit(&msg, sizeof(msg));
}

unsigned long get_filecrc(void)
{
    filecrc_msg_t crcmsg;

    send_message(MSG_CODE_SENDCRC, 0, 0);
    uart_receive(&crcmsg, sizeof(filecrc_msg_t));

    if(crcmsg.magic != MSG_MAGIC){
        printf("Bad magic number in get_filecrc()\n");
        die();
    }

    return crcmsg.crc;
}

bool get_fileinfo(fileinfo_msg_t *fi)
{
    int attempts = 10;
    while(attempts--){
        send_message(MSG_CODE_FILEINFO, 0, 0);
        uart_receive(fi, sizeof(fileinfo_msg_t));

        if(fi->magic != MSG_MAGIC){
            printf("get_fileinfo: Bad magic number\n");
        }else if(crc32((char*)fi, sizeof(fileinfo_msg_t)-sizeof(unsigned long), 0xFFFFFFFFUL) != ~fi->msg_crc){
            printf("get_fileinfo: Bad CRC32\n");
        }else{
            if(fi->filename[0] == 0 && fi->blocks == 0)
                return false; /* no more files */
            else
                return true;
        }
    }

    printf("Too many errors, giving up.\n");
    die();
    return false; /* this will never happen, but the compiler lives in hope and complains */
}

void send_file_done(bool received_okay)
{
    send_message(MSG_CODE_FILEDONE, received_okay ? 1 : 0, 0);
}

void send_block_request(unsigned int first_block, unsigned int count, fileblock_msg_t *buffer)
{
    /* prepare the receive side *first* so we're not racing with the transmitter */
    prepare_dma_receive(buffer, sizeof(fileblock_msg_header_t) + (CPM_BLOCK_SIZE * count));
    /* now send the request to the transmitter */
    send_message(MSG_CODE_SENDBLOCK, first_block, count);
}

void main()
{
    unsigned int blocks_remaining, request_count, request_first, complete_count, complete_first, i, file_block;
    char *write_ptr;
    unsigned long crc, sender_crc;
    bool file_okay;
    fileinfo_msg_t fileinfo;
    fileblock_msg_t *buffer_inflight, *buffer_complete, *buffer_temp;
    cpm_fcb fcb;

    printf("FATPIPE by Will Sowerbutts <will@sowerbutts.com> version 1.0.0\n\n");

    reset_dma();
    configure_uart();

    send_message(MSG_CODE_HELLO, 0, 0);

    while(get_fileinfo(&fileinfo)){
        printf("Receiving %-12s %4d KB   ", fileinfo.filename, fileinfo.blocks >> 3);

        cpm_f_prepare(&fcb, fileinfo.filename);
        cpm_f_delete(&fcb);
        if(cpm_f_create(&fcb)){
            printf("Cannot create file \"%s\"\n", fileinfo.filename);
            continue; /* let's try the next file? */
        }

        crc = 0xFFFFFFFFUL;
        blocks_remaining = fileinfo.blocks;
        complete_count = request_count = 0;
        complete_first = request_first = 0;
        buffer_inflight = &buffer_a;
        buffer_complete = &buffer_b;
        file_okay = true;

        while(file_okay){
            /* if we have data in flight, wait for it to arrive before requesting more (DMA unit will be busy) */
            if(complete_count > 0)
                wait_receive_complete();

            /* request as many blocks as we have space for into our buffer */
            request_count = (blocks_remaining > CPM_BLOCKS_PER_MESSAGE) ? CPM_BLOCKS_PER_MESSAGE : blocks_remaining;
            if(request_count > 0){
                send_block_request(request_first, request_count, buffer_inflight);
            }

            /* DMA will now fill buffer_inflight independently of the CPU */

            /* now process the previously received message (if any) */
            if(complete_count > 0){
                if(buffer_complete->header.magic != MSG_MAGIC){
                    printf("Bad magic number in received blocks\n");
                    file_okay = false;
                }else{
                    /* update CRC */
                    crc = crc32(buffer_complete->data, CPM_BLOCK_SIZE * complete_count, crc);

                    /* write to disk */
                    file_block = complete_first;
                    write_ptr = buffer_complete->data;
                    for(i=0; i<complete_count; i++){
                        if(cpm_f_write_random(&fcb, file_block, write_ptr)){
                            printf("CP/M write() failed\n");
                            die();
                        }
                        write_ptr += CPM_BLOCK_SIZE;
                        file_block++;
                    }
                }
            }

            printf("\x08%c", spinner());

            if(request_count == 0 && complete_count == 0){
                /* we're done */
                break;
            }

            /* now prepare for the next block */
            complete_count = request_count;
            complete_first = request_first;
            request_first += request_count;
            blocks_remaining -= request_count;
            buffer_temp = buffer_inflight;
            buffer_inflight = buffer_complete;
            buffer_complete = buffer_temp;
        }

        printf("\x08");
        cpm_f_close(&fcb);

        /* complete our CRC calculation */
        crc = ~crc;
        sender_crc = get_filecrc(); /* retrieve sender's computed CRC */

        if(crc != sender_crc){
            printf("CRC failure (received %08lx, sent %08lx) -- removing file.\n", crc, sender_crc);
            cpm_f_delete(&fcb);
            send_file_done(false);
        }else{
            printf("CRC32 OK!\n", crc);
            send_file_done(true);
        }
    }

    /* just to be sure */
    reset_dma();
}
