        .module buffers

        .globl _buffer_a
        .globl _buffer_b

; sdcc doesn't put buffers into _BSS so we end up huge chunks of nothing in our executable.
; we have to fix this up by hand.


; _BSS is located by the linker at 0x8000 so it does not get swapped out by the HBIOS and no double-copying is required
        .area   _BSS
        ; note the booster does not copy data in this section

        ; keep these in sync with the definitions in buffers.h
_buffer_a: .ds (2+(128 * 64))
_buffer_b: .ds (2+(128 * 64))
