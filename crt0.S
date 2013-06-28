/* -*- c -*- */

#define TRAP(H)  mov %psr, %l0; sethi %hi(H), %l4; jmp %l4+%lo(H); nop;
#define TRAP_ENTRY(H) rd %psr, %l0; b H; rd %wim, %l3; nop;
#define TRAP_ENTRY_INTERRUPT(int_level) \
        mov int_level, %l7; rd %psr, %l0; b _prom_leonbare_irq_entry; rd %wim, %l3;
#define BAD_TRAP ta 0; nop; nop; nop;

.data
.globl _leon_traphandlers

.section .text.init, "ax"
.global _start
.global __init_stack

/*
 * Trap handler table -> must be aligned to page size
 * as specified by the SPARC v8 manual (p. 31).
 */
.align(0x1000)
_leon_traphandlers:
  BAD_TRAP                       // 0x00
  BAD_TRAP                       // 0x01
  BAD_TRAP                       // 0x02
  BAD_TRAP                       // 0x03
  BAD_TRAP                       // 0x04
  TRAP(sparc_window_overflow);   // 0x05
  TRAP(sparc_window_underflow);  // 0x06
  BAD_TRAP                       // 0x07
  BAD_TRAP                       // 0x08
  TRAP(sparc_data_exception);    // 0x09
  BAD_TRAP                       // 0x0a
  BAD_TRAP                       // 0x0b
  BAD_TRAP                       // 0x0c
  BAD_TRAP                       // 0x0d
  BAD_TRAP                       // 0x0e
  BAD_TRAP                       // 0x0f
  BAD_TRAP                       // 0x10
  TRAP(irq_handler)              // 0x11 # interrupt level 1
  TRAP(irq_handler)              // 0x12
  TRAP(irq_handler)              // 0x13
  TRAP(irq_handler)              // 0x14
  TRAP(irq_handler)              // 0x15
  TRAP(irq_handler)              // 0x16
  TRAP(irq_handler)              // 0x17
  TRAP(irq_handler)              // 0x18
  TRAP(irq_handler)              // 0x19
  TRAP(irq_handler)              // 0x1a
  TRAP(irq_handler)              // 0x1b
  TRAP(irq_handler)              // 0x1c
  TRAP(irq_handler)              // 0x1d
  TRAP(irq_handler)              // 0x1e
  TRAP(irq_handler)              // 0x1f # interrupt level 15

_start:

  rd %asr17, %g1

  /*
   * setup trap handler table
   */
  sethi %hi(_leon_traphandlers), %g2
  wr %g2, %tbr

  /* the TBR setup above comes into effect three instructions from now!
   * Right now, we assume that no trap occurs in between.
   */

  /* set stack pointer */
  sethi %hi(__init_stack), %sp
  or %sp, %lo(__init_stack), %sp

  /*
   * setup task with enough space for registers %l0-%l7 and %i0-%i7
   */
  sub %sp, 64, %sp

  sethi %hi(__init_stack), %fp
  or %fp, %lo(__init_stack), %fp

  ba bootstrap_main
  nop

  ta 0
1:
  ba 1b


.globl sparc_window_overflow
sparc_window_overflow:
  mov %wim, %l3  /* need to determine new WIM */
  mov %g1, %l7
  srl %l3, 1, %g1

  /*
   * Find out if we are on LEON3 (PSR[24:27] == 3)
   * or on LEON2. For LEON3, we can read the number of
   * register windows from ASR17
   */
  mov %psr, %l4
  srl %l4, 24, %l4
  and %l4, 3, %l4
  subcc %l4, 3, %g0
  bne 1f
  nop

  /*
   * It's a LEON3
   */
  mov %asr17, %l4

  /* calculate new WIM */
  and %l4, 0x1f, %l4
  sll %l3, %l4, %l4
  or %l4, %g1, %g1

  /*
   * The trick here is to move to a valid stack frame
   * and store the register window contents there.
   */
  save
     mov %g1, %wim
     nop; nop; nop

     std %l0, [%sp + 0];
     std %l2, [%sp + 8];
     std %l4, [%sp + 16];
     std %l6, [%sp + 24];
     std %i0, [%sp + 32];
     std %i2, [%sp + 40];
     std %i4, [%sp + 48];
     std %i6, [%sp + 56];

  restore
  mov %l7, %g1
  jmp %l1
  rett %l2

1: ta 0

.globl sparc_window_underflow
sparc_window_underflow:
  mov %wim, %l3  /* need to determine new WIM */
  sll %l3, 1, %l4

  /* Determine LEON version */
  mov %psr, %l5
  srl %l5, 24, %l5
  and %l5, 3, %l5
  subcc %l5, 3, %g0
  bne 1f
  nop

  mov %asr17, %l5
  and %l5, 0x1f, %l5
  srl %l3, %l5, %l5
  or  %l5, %l4, %l5
  mov %l5, %wim
  nop; nop; nop

  restore             ! Two restores to get into the
  restore             ! window to restore
  ldd   [%sp + 0], %l0;       ! Restore window from the stack
  ldd   [%sp + 8], %l2;
  ldd   [%sp + 16], %l4;
  ldd   [%sp + 24], %l6;
  ldd   [%sp + 32], %i0;
  ldd   [%sp + 40], %i2;
  ldd   [%sp + 48], %i4;
  ldd   [%sp + 56], %i6;
  save                ! Get back to the trap window.
  save

  jmp %l1
  rett %l2

1: ta 0

.globl sparc_data_exception
sparc_data_exception:

  set 0x300, %l3  ! address of MMU fault status register
  set 0x400, %l4  ! address of MMU fault address register
  lda [%l4] 0x19, %o0 ! load fault address into 1st argument
  lda [%l3] 0x19, %o1 ! load status register into 2nd argument
  mov %l1, %o2        ! load faulting instruction into 3rd argument
  mov %sp, %o3        ! load sp into 4th argument
  
  ! re-enable traps
  mov %psr, %l0
  or %l0, 0x20, %l0
  mov %l0, %psr
  call pagefault_entry
  nop
  ! return to next PC
  jmpl %l2, %g0  ! old nPC
  rett %l2 + 4   ! old nPC + 4

.previous
.section .bss
.align 16
.space 4096
  __init_stack:
.previous


