IMPLEMENTATION [sparc]:

/** Note: TCB pointer is located in sprg1 */

/*
#include <cassert>
#include <cstdio>

#include "l4_types.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "space.h"
#include "thread_state.h"
*/

#include "kmem.h"
#include "utcb_support.h"
#include "warn.h"

IMPLEMENT inline
void
Context::spill_user_state()
{}

IMPLEMENT inline
void
Context::fill_user_state()
{}

PROTECTED inline void Context::arch_setup_utcb_ptr() {}

IMPLEMENT inline
void
Context::switch_cpu(Context *t)
{
  (void)t;
  NOT_IMPL_PANIC;
  // TODO flush register windows
  // TODO switch to new kernel stack
  // TODO call switchin_context_label
}

/** Thread context switchin.  Called on every re-activation of a
    thread (switch_exec()).  This method is public only because it is 
    called by an ``extern "C"'' function that is called
    from assembly code (call_switchin_context).
 */

IMPLEMENT
void Context::switchin_context(Context *from)
{
  (void)from;
  NOT_IMPL_PANIC;
}
