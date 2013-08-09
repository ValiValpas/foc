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

extern Mword *ksp;

IMPLEMENT inline
void
Context::spill_user_state()
{
  Entry_frame *ef = regs();
  assert_kdb (current() == this);
  ef->sp(Proc::stack_pointer());
  ef->ura = Proc::return_address();
}

IMPLEMENT inline
void
Context::fill_user_state()
{
  Entry_frame *ef = regs();
  Proc::stack_pointer(ef->sp());
  Proc::return_address(ef->ura);
}

PROTECTED inline void Context::arch_setup_utcb_ptr() {}

IMPLEMENT inline
void
Context::switch_cpu(Context *t)
{
  update_consumed_time();
  
  // flush register windows (saves current context(s) on the stack)
  Proc::flush_regwins();

  asm volatile
  (
    // get stack space for restart address (stack must be dword aligned)
    "sub   %sp, 8, %sp              \n"
    // save restart address on stack
    "sethi %hi(ra), %g1             \n"
    "or    %g1, %lo(ra), %g1        \n"
    "st    %g1, [%sp]               \n"
  );

  // save current stack pointer
  _kernel_sp = (Mword*)Proc::stack_pointer();

  // switch to new stack (and reserve an additional stack frame so that
  // we preserve the return context)
  Proc::stack_pointer((Mword)t->_kernel_sp - Config::Stack_frame_size);

  t->switchin_context(this);

  // load restart address from stack (this may either be "ra:" or 
  // user_invoke() if the thread didn't run before
  asm volatile
  (
    "ld  [%%sp + %[frame_size]], %%g1                \n"
    // jump to restart address
    "jmp %%g1                                        \n"
    // do something useful in the branch delay slot:
    "add %%sp, %[offset], %%sp                       \n"
    :
    : [offset]     "n" (Config::Stack_frame_size + 8),
      [frame_size] "n" (Config::Stack_frame_size)
  );

  asm volatile ("ra:");
  // load fp from stack
  Proc::frame_pointer(((Stack_frame*)Proc::stack_pointer())->i6);
  // load return address from stack
  Proc::return_address(((Stack_frame*)Proc::stack_pointer())->i7);

  printf("ra=0x%08lx, fp=0x%08lx\n", Proc::return_address(), Proc::frame_pointer());
  
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

  // switch to our page directory if nessecary
  vcpu_aware_space()->switchin_context(from->vcpu_aware_space());

  // stolen from arm implementation:
  Utcb_support::current(current()->utcb().usr());
  // update kernel entry stack pointer
  ksp = (Mword*)current()->regs();
}
