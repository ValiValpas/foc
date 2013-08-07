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
.global _start_kernel
.global start
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
  TRAP_ENTRY_INTERRUPT(0x1)      // 0x11 # interrupt level 1
  TRAP_ENTRY_INTERRUPT(0x2)      // 0x12
  TRAP_ENTRY_INTERRUPT(0x3)      // 0x13
  TRAP_ENTRY_INTERRUPT(0x4)      // 0x14
  TRAP_ENTRY_INTERRUPT(0x5)      // 0x15
  TRAP_ENTRY_INTERRUPT(0x6)      // 0x16
  TRAP_ENTRY_INTERRUPT(0x7)      // 0x17
  TRAP_ENTRY_INTERRUPT(0x8)      // 0x18
  TRAP_ENTRY_INTERRUPT(0x9)      // 0x19
  TRAP_ENTRY_INTERRUPT(0xa)      // 0x1a
  TRAP_ENTRY_INTERRUPT(0xb)      // 0x1b
  TRAP_ENTRY_INTERRUPT(0xc)      // 0x1c
  TRAP_ENTRY_INTERRUPT(0xd)      // 0x1d
  TRAP_ENTRY_INTERRUPT(0xe)      // 0x1e
  TRAP_ENTRY_INTERRUPT(0xf)      // 0x1f # interrupt level 15
  BAD_TRAP                       // 0x20
  BAD_TRAP                       // 0x21
  BAD_TRAP                       // 0x22
  BAD_TRAP                       // 0x23
  BAD_TRAP                       // 0x24
  BAD_TRAP                       // 0x25
  BAD_TRAP                       // 0x26
  BAD_TRAP                       // 0x27
  BAD_TRAP                       // 0x28
  BAD_TRAP                       // 0x29
  BAD_TRAP                       // 0x2a
  BAD_TRAP                       // 0x2b
  BAD_TRAP                       // 0x2c
  BAD_TRAP                       // 0x2d
  BAD_TRAP                       // 0x2e
  BAD_TRAP                       // 0x2f
  BAD_TRAP                       // 0x30
  BAD_TRAP                       // 0x31
  BAD_TRAP                       // 0x32
  BAD_TRAP                       // 0x33
  BAD_TRAP                       // 0x34
  BAD_TRAP                       // 0x35
  BAD_TRAP                       // 0x36
  BAD_TRAP                       // 0x37
  BAD_TRAP                       // 0x38
  BAD_TRAP                       // 0x39
  BAD_TRAP                       // 0x3a
  BAD_TRAP                       // 0x3b
  BAD_TRAP                       // 0x3c
  BAD_TRAP                       // 0x3d
  BAD_TRAP                       // 0x3e
  BAD_TRAP                       // 0x3f
  BAD_TRAP                       // 0x40
  BAD_TRAP                       // 0x41
  BAD_TRAP                       // 0x42
  BAD_TRAP                       // 0x43
  BAD_TRAP                       // 0x44
  BAD_TRAP                       // 0x45
  BAD_TRAP                       // 0x46
  BAD_TRAP                       // 0x47
  BAD_TRAP                       // 0x48
  BAD_TRAP                       // 0x49
  BAD_TRAP                       // 0x4a
  BAD_TRAP                       // 0x4b
  BAD_TRAP                       // 0x4c
  BAD_TRAP                       // 0x4d
  BAD_TRAP                       // 0x4e
  BAD_TRAP                       // 0x4f
  BAD_TRAP                       // 0x50
  BAD_TRAP                       // 0x51
  BAD_TRAP                       // 0x52
  BAD_TRAP                       // 0x53
  BAD_TRAP                       // 0x54
  BAD_TRAP                       // 0x55
  BAD_TRAP                       // 0x56
  BAD_TRAP                       // 0x57
  BAD_TRAP                       // 0x58
  BAD_TRAP                       // 0x59
  BAD_TRAP                       // 0x5a
  BAD_TRAP                       // 0x5b
  BAD_TRAP                       // 0x5c
  BAD_TRAP                       // 0x5d
  BAD_TRAP                       // 0x5e
  BAD_TRAP                       // 0x5f
  BAD_TRAP                       // 0x60
  BAD_TRAP                       // 0x61
  BAD_TRAP                       // 0x62
  BAD_TRAP                       // 0x63
  BAD_TRAP                       // 0x64
  BAD_TRAP                       // 0x65
  BAD_TRAP                       // 0x66
  BAD_TRAP                       // 0x67
  BAD_TRAP                       // 0x68
  BAD_TRAP                       // 0x69
  BAD_TRAP                       // 0x6a
  BAD_TRAP                       // 0x6b
  BAD_TRAP                       // 0x6c
  BAD_TRAP                       // 0x6d
  BAD_TRAP                       // 0x6e
  BAD_TRAP                       // 0x6f
  BAD_TRAP                       // 0x70
  BAD_TRAP                       // 0x71
  BAD_TRAP                       // 0x72
  BAD_TRAP                       // 0x73
  BAD_TRAP                       // 0x74
  BAD_TRAP                       // 0x75
  BAD_TRAP                       // 0x76
  BAD_TRAP                       // 0x77
  BAD_TRAP                       // 0x78
  BAD_TRAP                       // 0x79
  BAD_TRAP                       // 0x7a
  BAD_TRAP                       // 0x7b
  BAD_TRAP                       // 0x7c
  BAD_TRAP                       // 0x7d
  BAD_TRAP                       // 0x7e
  BAD_TRAP                       // 0x7f
  BAD_TRAP                       // 0x80
  BAD_TRAP                       // 0x81
  BAD_TRAP                       // 0x82
  BAD_TRAP                       // 0x83
  BAD_TRAP                       // 0x84
  BAD_TRAP                       // 0x85
  BAD_TRAP                       // 0x86
  BAD_TRAP                       // 0x87
  BAD_TRAP                       // 0x88
  BAD_TRAP                       // 0x89
  BAD_TRAP                       // 0x8a
  BAD_TRAP                       // 0x8b
  BAD_TRAP                       // 0x8c
  BAD_TRAP                       // 0x8d
  BAD_TRAP                       // 0x8e
  BAD_TRAP                       // 0x8f
  BAD_TRAP                       // 0x90
  BAD_TRAP                       // 0x91
  BAD_TRAP                       // 0x92
  BAD_TRAP                       // 0x93
  BAD_TRAP                       // 0x94
  BAD_TRAP                       // 0x95
  BAD_TRAP                       // 0x96
  BAD_TRAP                       // 0x97
  BAD_TRAP                       // 0x98
  BAD_TRAP                       // 0x99
  BAD_TRAP                       // 0x9a
  BAD_TRAP                       // 0x9b
  BAD_TRAP                       // 0x9c
  BAD_TRAP                       // 0x9d
  BAD_TRAP                       // 0x9e
  BAD_TRAP                       // 0x9f
  BAD_TRAP                       // 0xa0
  BAD_TRAP                       // 0xa1
  BAD_TRAP                       // 0xa2
  BAD_TRAP                       // 0xa3
  BAD_TRAP                       // 0xa4
  BAD_TRAP                       // 0xa5
  BAD_TRAP                       // 0xa6
  BAD_TRAP                       // 0xa7
  BAD_TRAP                       // 0xa8
  BAD_TRAP                       // 0xa9
  BAD_TRAP                       // 0xaa
  BAD_TRAP                       // 0xab
  BAD_TRAP                       // 0xac
  BAD_TRAP                       // 0xad
  BAD_TRAP                       // 0xae
  BAD_TRAP                       // 0xaf
  BAD_TRAP                       // 0xb0
  BAD_TRAP                       // 0xb1
  BAD_TRAP                       // 0xb2
  BAD_TRAP                       // 0xb3
  BAD_TRAP                       // 0xb4
  BAD_TRAP                       // 0xb5
  BAD_TRAP                       // 0xb6
  BAD_TRAP                       // 0xb7
  BAD_TRAP                       // 0xb8
  BAD_TRAP                       // 0xb9
  BAD_TRAP                       // 0xba
  BAD_TRAP                       // 0xbb
  BAD_TRAP                       // 0xbc
  BAD_TRAP                       // 0xbd
  BAD_TRAP                       // 0xbe
  BAD_TRAP                       // 0xbf
  BAD_TRAP                       // 0xc0
  BAD_TRAP                       // 0xc1
  BAD_TRAP                       // 0xc2
  BAD_TRAP                       // 0xc3
  BAD_TRAP                       // 0xc4
  BAD_TRAP                       // 0xc5
  BAD_TRAP                       // 0xc6
  BAD_TRAP                       // 0xc7
  BAD_TRAP                       // 0xc8
  BAD_TRAP                       // 0xc9
  BAD_TRAP                       // 0xca
  BAD_TRAP                       // 0xcb
  BAD_TRAP                       // 0xcc
  BAD_TRAP                       // 0xcd
  BAD_TRAP                       // 0xce
  BAD_TRAP                       // 0xcf
  BAD_TRAP                       // 0xd0
  BAD_TRAP                       // 0xd1
  BAD_TRAP                       // 0xd2
  BAD_TRAP                       // 0xd3
  BAD_TRAP                       // 0xd4
  BAD_TRAP                       // 0xd5
  BAD_TRAP                       // 0xd6
  BAD_TRAP                       // 0xd7
  BAD_TRAP                       // 0xd8
  BAD_TRAP                       // 0xd9
  BAD_TRAP                       // 0xda
  BAD_TRAP                       // 0xdb
  BAD_TRAP                       // 0xdc
  BAD_TRAP                       // 0xdd
  BAD_TRAP                       // 0xde
  BAD_TRAP                       // 0xdf
  BAD_TRAP                       // 0xe0
  BAD_TRAP                       // 0xe1
  BAD_TRAP                       // 0xe2
  BAD_TRAP                       // 0xe3
  BAD_TRAP                       // 0xe4
  BAD_TRAP                       // 0xe5
  BAD_TRAP                       // 0xe6
  BAD_TRAP                       // 0xe7
  BAD_TRAP                       // 0xe8
  BAD_TRAP                       // 0xe9
  BAD_TRAP                       // 0xea
  BAD_TRAP                       // 0xeb
  BAD_TRAP                       // 0xec
  BAD_TRAP                       // 0xed
  BAD_TRAP                       // 0xee
  BAD_TRAP                       // 0xef
  BAD_TRAP                       // 0xf0
  BAD_TRAP                       // 0xf1
  BAD_TRAP                       // 0xf2
  BAD_TRAP                       // 0xf3
  BAD_TRAP                       // 0xf4
  BAD_TRAP                       // 0xf5
  BAD_TRAP                       // 0xf6
  BAD_TRAP                       // 0xf7
  BAD_TRAP                       // 0xf8
  BAD_TRAP                       // 0xf9
  BAD_TRAP                       // 0xfa
  BAD_TRAP                       // 0xfb
  BAD_TRAP                       // 0xfc
  BAD_TRAP                       // 0xfd
  BAD_TRAP                       // 0xfe
  BAD_TRAP                       // 0xff

_start_kernel:
start:

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

  ba __main
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

  /* Some considerations about reliability:
   * In case the stack pointer of the register window has
   * been corrupted, we might get a data exception which
   * consequently crashes the entire system (traps disabled),
   * i.e. the CPU will go into error mode and halt execution.
   * 
   * In order to prevent this, we could either handle the 
   * error mode somehow or do the following:
   * 
   *   save         ! go into window to be stored
   *   mov %sp, %g2
   *   restore      ! go into trap window
   *   <enable traps>
   *   restore      ! go into previous window
   *   ld %g2, %g3  ! load from sp
   *   st %g3, %g2  ! store sp 
   *   save         ! go back into trap window
   * 
   * This way we ensure that a data exception can use the trap
   * window.
   * We also just need to enable traps in the underflow handler
   * to make sure we can handle data exceptions in RESTORE_FRAME().
   *
   * TODO prevent data access exceptions in overflow/underflow handler
   */

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
  or %t_psr, 0xF20, %l6 ! set ET bit and disable IRQs
  wr %l6, %psr
  call pagefault_entry
  nop
  ! restore psr (clear ET bit by xor-ing)
  wr %t_psr, %psr
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
  STORE_GLOBALS(sp, l6)

  ! prepare arguments
  mov %t_psr, %o0  ! psr
  mov %t_pc,  %o1  ! PC
  mov %t_npc, %o2  ! nPC
  mov %l7, %o3     ! int lvl

  ! check whether we are in the trap window (see etrap_fast.S from gaisler)
  mov 1, %g1
  sll %g1, %t_psr, %g1  ! 1 << psr[4:0]
  andcc %g1, %t_wim, %g0
  beq 1f
   nop

    ! in trap window, clean up
    ! compute new window
    srl %t_wim, 0x1, %g2
    sll %t_wim, 7, %t_wim
    or %t_wim, %g2, %g2
    and %g2, 0xff, %g2

    ! get in window to be saved
    save
      wr %g2, %wim
      STORE_FRAME(sp)
    restore

1:
  ! re-enable traps and disable irqs
  or %t_psr, 0xF20, %l6
  wr %l6, %psr

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
  be	2f		            ! Nope, just return from the trap
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
      b	3f
    save

2:
  ! restore globals and %y from stack
  RESTORE_GLOBALS(sp, l6)

3:
  ! return
  wr %t_psr, %psr ! restore psr
  nop; nop; nop;

  jmp %t_pc
  rett %t_npc

.previous
.section .bss
.align 16
.space 4096
  __init_stack:
.previous
