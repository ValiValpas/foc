.globl l4_atomic_add
l4_atomic_add:
  ! skip if we add zero
  cmp %o1, %g0
  beq 99f
  ld [%o0], %g1
10:
  add %g1, %o1, %g2
  mov %g2, %g3
  casa [%o0] 0xA, %g1, %g2
  cmp %g1, %g2
  bne 10b
   mov %g2, %g1
99:
  retl
   mov %g3, %o0

.globl l4_atomic_cmpxchg
l4_atomic_cmpxchg:
  casa [%o0] 0xA, %o1, %o2 
! better do this:
! xor %o1, %o2, %o1
! cmp %g0, %o1
! retl
! subx %g0, -1, %o0
  cmp %o1, %o2
  bne 99f
   clr %o0

    retl 
     inc %o0
99:
  retl
   nop

.globl l4_atomic_xchg
l4_atomic_xchg:
  swap [%o0], %o1
  retl
   mov %o1, %o0
