SDAS=sdasz80
SDCC=sdcc
SDLD=sdldz80
SDASOPTS=-plosff

SDCCOPTS=--std-sdcc99 --no-std-crt0 -mz180 --opt-code-size --max-allocs-per-node 25000 --Werror --stack-auto

FATPIPE_OBJS  = runtime0.rel libcpm2.rel fatpipe.rel  libcpm.rel crc32.rel buffers.rel
CRCFILE_OBJS  = runtime0.rel libcpm2.rel crcfile.rel  libcpm.rel crc32.rel
WRITEBIG_OBJS = runtime0.rel libcpm2.rel writebig.rel libcpm.rel
TEST_OBJS     = runtime0.rel libcpm2.rel test.rel     libcpm.rel

TARGETS	= fatpipe.com crcfile.com writebig.com test.com

all:	$(TARGETS)

.SUFFIXES:		# delete the default suffixes
.SUFFIXES: .c .s .rel

%.rel: %.c
	$(SDCC) $(SDCCOPTS) -c $<

%.rel: %.s
	$(SDAS) $(SDASOPTS) $<

.PHONY:	clean
clean:
	rm -f *~ *.ihx *.hex *.rel *.map *.bin *.noi *.lst *.asm *.sym $(TARGETS)

fatpipe.com: $(FATPIPE_OBJS)
	$(SDLD) -nmwx -i fatpipe.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(FATPIPE_OBJS)
	srec_cat -disable-sequence-warning fatpipe.ihx -intel -offset -0x100 -output fatpipe.com -binary

crcfile.com: $(CRCFILE_OBJS)
	$(SDLD) -nmwx -i crcfile.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(CRCFILE_OBJS)
	srec_cat -disable-sequence-warning crcfile.ihx -intel -offset -0x100 -output crcfile.com -binary

writebig.com: $(WRITEBIG_OBJS)
	$(SDLD) -nmwx -i writebig.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(WRITEBIG_OBJS)
	srec_cat -disable-sequence-warning writebig.ihx -intel -offset -0x100 -output writebig.com -binary

test.com: $(TEST_OBJS)
	$(SDLD) -nmwx -i test.ihx -b _CODE=0x100 -b _BSS=0x8000 -k /usr/share/sdcc/lib/z180/ -l z180 $(TEST_OBJS)
	srec_cat -disable-sequence-warning test.ihx -intel -offset -0x100 -output test.com -binary
