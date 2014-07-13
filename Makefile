SDAS=sdasz80
SDCC=sdcc
SDLD=sdldz80
SDASOPTS=-plosff

SDCCOPTS=--std-sdcc99 --no-std-crt0 -mz180 --opt-code-size --max-allocs-per-node 25000 --Werror --stack-auto

FATPIPE_CSRCS =  libcpm2.c fatpipe.c
FATPIPE_ASRCS =  runtime0.s putchar.s libcpm.s crc32.s buffers.s
FATPIPE_OBJS  =  $(FATPIPE_ASRCS:.s=.rel) $(FATPIPE_CSRCS:.c=.rel)

CRCFILE_CSRCS =  libcpm2.c crcfile.c
CRCFILE_ASRCS =  runtime0.s putchar.s libcpm.s crc32.s
CRCFILE_OBJS  =  $(CRCFILE_ASRCS:.s=.rel) $(CRCFILE_CSRCS:.c=.rel)

JUNK = $(CRCFILE_CSRCS:.c=.lst) $(CRCFILE_CSRCS:.c=.asm) $(CRCFILE_CSRCS:.c=.sym) $(CRCFILE_ASRCS:.s=.lst) $(CRCFILE_ASRCS:.s=.sym) $(CRCFILE_CSRCS:.c=.rst) $(CRCFILE_ASRCS:.s=.rst) $(FATPIPE_CSRCS:.c=.lst) $(FATPIPE_CSRCS:.c=.asm) $(FATPIPE_CSRCS:.c=.sym) $(FATPIPE_ASRCS:.s=.lst) $(FATPIPE_ASRCS:.s=.sym) $(FATPIPE_CSRCS:.c=.rst) $(FATPIPE_ASRCS:.s=.rst)

all:	fatpipe.com crcfile.com

.SUFFIXES:		# delete the default suffixes
.SUFFIXES: .c .s .rel

%.rel: %.c
	$(SDCC) $(SDCCOPTS) -c $<

%.rel: %.s
	$(SDAS) $(SDASOPTS) $<

clean:
	rm -f $(FATPIPE_OBJS) $(JUNK) *~ fatpipe.com fatpipe.ihx fatpipe.map crcfile.com crcfile.ihx crcfile.map crcfile.rel

fatpipe.com: $(FATPIPE_OBJS)
	$(SDLD) -nmwx -i fatpipe.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(FATPIPE_OBJS)
	srec_cat -disable-sequence-warning fatpipe.ihx -intel -offset -0x100 -output fatpipe.com -binary

crcfile.com: $(CRCFILE_OBJS)
	$(SDLD) -nmwx -i crcfile.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(CRCFILE_OBJS)
	srec_cat -disable-sequence-warning crcfile.ihx -intel -offset -0x100 -output crcfile.com -binary
