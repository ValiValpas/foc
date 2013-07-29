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
{
  Entry_frame *ef = regs();
  assert_kdb (current() == this);
  ef->sp(Proc::stack_pointer());
  ef->i7 = Proc::return_address();
}

IMPLEMENT inline
void
Context::fill_user_state()
{
  Entry_frame *ef = regs();
  Proc::stack_pointer(ef->sp());
  Proc::return_address(ef->i7);
}

PROTECTED inline void Context::arch_setup_utcb_ptr() {}

IMPLEMENT inline
void
Context::switch_cpu(Context *t)
{
  update_consumed_time();

  // save current stack pointer
  _kernel_sp = (Mword*)Proc::stack_pointer();

  // flush register windows (saves current context(s) on the stack)
  Proc::flush_regwins();

  // switch to new stack
  Proc::stack_pointer((Mword)t->_kernel_sp);

  // load i7 from stack
  // remark: we can use the Return_frame to access the registers on the stack
  // because it is laid out accordingly
  Proc::return_address(((Return_frame*)Proc::stack_pointer())->i7);
  // load fp from stack
  Proc::frame_pointer(((Return_frame*)Proc::stack_pointer())->i6);

  t->switchin_context(this);

  
  // at this point a ret and restore will cause an underflow trap
  // which, in turn, restores a register window from the new stack
}

/** Thread context switchin.  Called on every re-activation of a
    thread (switch_exec()).  This method is public only because it is 
    called by an ``extern "C"'' function that is called
    from assembly code (call_switchin_context).
 */

IMPLEMENT
void Context::switchin_context(Context *from)
{
  assert_kdb (this == current());
  assert_kdb (state() & Thread_ready_mask);

  // FIXME the ia32 implementation does some sp manipulations here

  printf("switchin, switchin (to=0x%08lx)\n", vcpu_aware_space());
  printf("switchin, switchin (from=0x%08lx)\n", from->vcpu_aware_space());
  // switch to our page directory if nessecary
  vcpu_aware_space()->switchin_context(from->vcpu_aware_space());

  printf("..., switchin\n");
  // stolen from arm implementation:
  Utcb_support::current(current()->utcb().usr());
}
