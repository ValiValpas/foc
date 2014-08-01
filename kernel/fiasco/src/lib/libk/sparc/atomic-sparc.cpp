IMPLEMENTATION [sparc && leon3]:

inline
bool
cas_unsafe( Mword *ptr, Mword oldval, Mword newval )
{
  // for leon3 we can use the casa instruction
  asm volatile ( "cas [%[ptr]], %[oldval], %[newval]"
                : [newval] "+r" (newval)
                : [ptr] "r" (ptr), [oldval] "r" (oldval)
                : "memory");

  return (oldval == newval);
}

//----------------------------------------------------------------------
IMPLEMENTATION [sparc && !leon3]:
#include "cpu_lock.h"
#include "lock_guard.h"

inline NEEDS["cpu_lock.h", "lock_guard.h"]
bool
cas_unsafe( Mword *ptr, Mword oldval, Mword newval )
{
  // there is no compare-and-swap on sparcV8, i.e.
  // we need to lock the cpu for the non-mp cas

  auto guard = lock_guard(cpu_lock);
  if (*ptr == oldval) {
    asm volatile ("swap %0, %1"
                  : /* no output */
                  : "m" (*ptr), "r" (newval)
                  : "memory");
    return true;
  }

  return false;
}

//----------------------------------------------------------------------
IMPLEMENTATION [sparc]:
#include <panic.h>
#include <stdio.h>

/* dummy implement */
bool
cas2_unsafe( Mword *, Mword *, Mword *)
{
  panic("%s not implemented", __func__);
  return false;
}

inline
void
atomic_and (Mword *l, Mword mask)
{
  Mword old;
  do { old = *l; }
  while ( !cas (l, old, old & mask));
}

inline
void
atomic_or (Mword *l, Mword bits)
{
  Mword old;
  do { old = *l; }
  while ( !cas (l, old, old | bits));
}

IMPLEMENTATION [(sparc && mp)]:

inline
bool
mp_cas_arch(Mword *, Mword, Mword)
{
  // FIXME implement sparc mp_cas_arch()
  // i.e. emulate compare-and-swap with spinlocks
  panic("%s not implemented", __func__);
  return false;
}
