/* -*- c -*- */

#define TRAP(H)  mov %psr, %l0; sethi %hi(H), %l4; jmp %l4+%lo(H); nop;
#define TRAP_ENTRY(H) rd %psr, %l0; b H; rd %wim, %l3; nop;
#define TRAP_ENTRY_INTERRUPT(int_level) \
        mov int_level, %l7; rd %psr, %l0; b irq_entry; rd %wim, %l3;
#define BAD_TRAP ta 0; nop; nop; nop;

#define t_psr l0
#define t_pc  l1
#define t_npc l2
#define t_wim l3

! Additional stack space for non-flat traps
! (96 for prospective window overflow, 32 for globals)
! maybe we just need an additional 32 bytes
#define STACK_ALLOC_SIZE 128

#define STORE_GLOBALS(base_reg, scratch) \
  rd  %y, %scratch;                \
  st  %scratch, [%base_reg + 96];  \
  st  %g1,      [%base_reg + 100]; \
  std %g2,      [%base_reg + 104]; \
  std %g4,      [%base_reg + 112]; \
  std %g6,      [%base_reg + 120];
  
#define RESTORE_GLOBALS(base_reg, scratch) \
  ld  [%base_reg + 96],  %scratch; \
  ld  [%base_reg + 100], %g1; \
  ldd [%base_reg + 104], %g2; \
  ldd [%base_reg + 112], %g4; \
  ldd [%base_reg + 120], %g6; \
  wr  %scratch, %y;

#define STORE_FRAME(base_reg) \
  std %l0, [%base_reg + 0]; \
  std %l2, [%base_reg + 8]; \
  std %l4, [%base_reg + 16]; \
  std %l6, [%base_reg + 24]; \
  std %i0, [%base_reg + 32]; \
  std %i2, [%base_reg + 40]; \
  std %i4, [%base_reg + 48]; \
  std %i6, [%base_reg + 56];
#define RESTORE_FRAME(base_reg) \
  ldd   [%base_reg + 0], %l0; \
  ldd   [%base_reg + 8], %l2; \
  ldd   [%base_reg + 16], %l4; \
  ldd   [%base_reg + 24], %l6; \
  ldd   [%base_reg + 32], %i0; \
  ldd   [%base_reg + 40], %i2; \
  ldd   [%base_reg + 48], %i4; \
  ldd   [%base_reg + 56], %i6; \

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
  TRAP_ENTRY_INTERRUPT(0x11)     // 0x11 # interrupt level 1
  TRAP_ENTRY_INTERRUPT(0x12)     // 0x12
  TRAP_ENTRY_INTERRUPT(0x13)     // 0x13
  TRAP_ENTRY_INTERRUPT(0x14)     // 0x14
  TRAP_ENTRY_INTERRUPT(0x15)     // 0x15
  TRAP_ENTRY_INTERRUPT(0x16)     // 0x16
  TRAP_ENTRY_INTERRUPT(0x17)     // 0x17
  TRAP_ENTRY_INTERRUPT(0x18)     // 0x18
  TRAP_ENTRY_INTERRUPT(0x19)     // 0x19
  TRAP_ENTRY_INTERRUPT(0x1a)     // 0x1a
  TRAP_ENTRY_INTERRUPT(0x1b)     // 0x1b
  TRAP_ENTRY_INTERRUPT(0x1c)     // 0x1c
  TRAP_ENTRY_INTERRUPT(0x1d)     // 0x1d
  TRAP_ENTRY_INTERRUPT(0x1e)     // 0x1e
  TRAP_ENTRY_INTERRUPT(0x1f)     // 0x1f # interrupt level 15

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
  mov %wim, %t_wim  /* need to determine new WIM */
  mov %g1, %l7
  srl %t_wim, 1, %g1

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
  and %l4, 0x1f, %l4 ! number of windows
  sll %t_wim, %l4, %l4
  or %l4, %g1, %g1

  /*
   * The trick here is to move to a valid stack frame
   * and store the register window contents there.
   */
  save
     mov %g1, %wim
     nop; nop; nop

     STORE_FRAME(sp)

  restore
  mov %l7, %g1
  jmp %t_pc
  rett %t_npc

1: ta 0

.globl sparc_window_underflow
sparc_window_underflow:
  mov %wim, %t_wim  /* need to determine new WIM */
  sll %t_wim, 1, %l4

  /* Determine LEON version */
  mov %psr, %l5
  srl %l5, 24, %l5
  and %l5, 3, %l5
  subcc %l5, 3, %g0
  bne 1f
  nop

  mov %asr17, %l5
  and %l5, 0x1f, %l5
  srl %t_wim, %l5, %l5
  or  %l5, %l4, %l5
  mov %l5, %wim
  nop; nop; nop

  restore             ! Two restores to get into the
  restore             ! window to restore
  RESTORE_FRAME(sp)   ! Restore window from the stack
  save                ! Get back to the trap window.
  save

  jmp %t_pc
  rett %t_npc

1: ta 0

.globl sparc_data_exception
sparc_data_exception:

  set 0x300, %l3  ! address of MMU fault status register
  set 0x400, %l4  ! address of MMU fault address register
  lda [%l4] 0x19, %o0 ! load fault address into 1st argument
  lda [%l3] 0x19, %o1 ! load status register into 2nd argument
  mov %t_pc, %o2        ! load faulting instruction into 3rd argument
  mov %sp,   %o3        ! load sp into 4th argument
  
  ! re-enable traps
  ! TODO be careful with nested traps (need to allocate a stack frame)
  mov %psr, %t_psr
  or %t_psr, 0x20, %t_psr ! set ET bit
  wr %t_psr, %psr
  call pagefault_entry
  nop
  ! restore psr (clear ET bit by xor-ing)
  wr %t_psr, 0x20, %psr
  nop; nop; nop;
  ! return to next PC
  jmpl %t_npc, %g0  ! old nPC
  rett %t_npc + 4   ! old nPC + 4

.globl irq_entry
irq_entry:
!  restore
!    STORE_FRAME() ! store return frame
!  save

  ! save globals and %y on stack
  ! TODO can we make this faster by using the local registers instead?
  add %fp, -STACK_ALLOC_SIZE, %sp   ! get some extra space on the stack
  STORE_FRAME(sp)      ! TODO this should be done within on a window overflow (but it does not work)
  STORE_GLOBALS(sp, l6)

  ! prepare arguments
  mov %t_psr, %o0  ! psr
  mov %t_pc,  %o1  ! PC
  mov %t_npc, %o2  ! nPC
  mov %l7, %o3     ! int lvl

  ! TODO check whether we are in the trap window (see etrap_fast.S from gaisler)

  ! re-enable traps
  or %t_psr, 0x20, %t_psr
  mov %t_psr, %psr

  ! do high-level stuff
  call irq_handler
  nop

	/* Will the rett land us in the invalid window? */
  mov	2, %g1
  sll	%g1, %t_psr, %g1  ! shift left by CWP (ignores 27 MSBs of t_psr)

  srl	%g1, 8, %g2       ! hard coded number of register windows (could be read from asr17)
  or	%g1, %g2, %g1
  rd	%wim, %g2
  andcc	%g2, %g1, %g0
  be	1f		            ! Nope, just return from the trap
   sll %g2, 0x1, %g1

    /* We have to grab a window before returning. */
    srl	%g2, 7, %g2
    or	%g1, %g2, %g1
    and	%g1, 0xff, %g1

    wr	%g1, 0x0, %wim

    /* Grrr, make sure we load from the right %sp... */
    RESTORE_GLOBALS(sp, l6)
	
    restore
      RESTORE_FRAME(sp)
      b	2f
    save

1:
  ! restore globals and %y from stack
  RESTORE_GLOBALS(sp, l6)
  RESTORE_FRAME(sp)  ! TODO see above

2:
  ! return
  wr %t_psr, 0x20, %psr ! restore psr
  nop; nop; nop;

  jmp %t_pc
  rett %t_npc

.previous
.section .bss
.align 16
.space 4096
  __init_stack:
.previous


