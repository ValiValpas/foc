INTERFACE [sparc]:

class Trap_state;

IMPLEMENTATION [sparc]:

#include <cassert>
#include <cstdio>
#include <feature.h>

#include "globals.h"
#include "kmem.h"
#include "thread_state.h"
#include "trap_state.h"
#include "types.h"

extern Mword *ksp;

//enum {
//  FSR_STATUS_MASK = 0x0d,
//  FSR_TRANSL      = 0x05,
//  FSR_DOMAIN      = 0x09,
//  FSR_PERMISSION  = 0x0d,
//};

DEFINE_PER_CPU Per_cpu<Thread::Dbg_stack> Thread::dbg_stack;

PRIVATE static
void
Thread::print_page_fault_error(Mword e)
{
  char const *const excpts[] =
    { "reset","undef. insn", "swi", "pref. abort", "data abort",
      "XXX", "XXX", "XXX" };

  unsigned ex = (e >> 20) & 0x07;

  printf("(%lx) %s, %s(%c)",e & 0xff, excpts[ex],
         (e & 0x00010000)?"user":"kernel",
         (e & 0x00020000)?'r':'w');
}

//
// Public services
//
PRIVATE static 
void
Thread::dump_bats()
{
}

PUBLIC template<typename T> inline
void FIASCO_NORETURN
Thread::fast_return_to_user(Mword ip, Mword sp, T arg)
{
  (void)ip; (void)sp; (void)arg;
  //assert_kdb(check that exiting privs are user privs);
  // XXX: UNIMPLEMENTED
  panic("__builtin_trap()");
}

IMPLEMENT
void
Thread::user_invoke()
{
  user_invoke_generic();
  assert(current()->state() & Thread_ready);

  Return_frame *r = nonull_static_cast<Return_frame*>(current()->regs());
  Kip *kip = (EXPECT_FALSE(current_thread()->mem_space()->is_sigma0())) ?
             (Kip*)Mem_space::kernel_space()->virt_to_phys((Address)Kip::k()) : 0;

  ksp = (Mword*)current()->regs();


  Proc::stack_pointer(r->sp());

  // restore psr
  Mword psr = (r->psr & Psr::Usr_ret_mask) | (1 << Psr::Enable_trap) | (Psr::read() & ~Psr::Usr_ret_mask);

  // As the delay of a write operation to psr is implementation-dependent, we 
  // do this in the syscall page because this is mapped executable in user space.
  // The code in the syscall page is the following:
  //  wrpsr %o7
  //  nop
  //  nop
  //  nop
  //  jmp %g1
  //  nop

  // 
  asm volatile
  (
    "mov %[psr], %%o7\n"
    "mov %[kip], %%o0 \n"
    "mov %[ret], %%g1 \n"
    "jmp %[sc]        \n"
	 "nop              \n"
    :
    : [ret] "r" (r->ip()),
      [psr] "r" (psr),
      [kip] "r" (kip),
		[sc]  "r" (Mem_layout::Syscalls)
    :
  );

  // never returns
  panic("Hit unreachable instruction in user_invoke()\n");
}

IMPLEMENT inline NEEDS["space.h", <cstdio>, "types.h" ,"config.h"]
bool Thread::handle_sigma0_page_fault(Address pfa)
{
  bool ret = (mem_space()->v_insert(Mem_space::Phys_addr(pfa & Config::SUPERPAGE_MASK),
				    Virt_addr(pfa & Config::SUPERPAGE_MASK),
				    Virt_order(Config::SUPERPAGE_SIZE),
				    Mem_space::Attr(L4_fpage::Rights::URWX()))
	!= Mem_space::Insert_err_nomem);

  return ret;
}

extern "C" {
  void except_notimpl(void)
  {
  }

};

extern "C" {

  /**
   * The low-level page fault handler called from crt0.S.  We're invoked with
   * traps turned on.
   * @param pfa page-fault virtual address
   * @param error_code MMU fault register
   * @return true if page fault could be resolved, false otherwise
   */
  Mword pagefault_entry(const Mword pfa, Mword error_code,
                        const Mword pc, Return_frame *ret_frame)
  {
    error_code &= ~Fsr::Reserved_mask;
    if (((Psr::read() >> Psr::Prev_superuser) & 0x1) == 0) {
      error_code |= 1 << Fsr::User_mode;
    }

    if(EXPECT_TRUE(PF::is_usermode_error(error_code)))
      {
        assert(((Psr::read() >> Psr::Prev_superuser) & 0x1) == 0);
        if (current_thread()->vcpu_pagefault(pfa, error_code, pc))
          return 1;

        current_thread()->state_del(Thread_cancel);
        // DON'T enable interrupts here because we currently don't support nested traps
//        Proc::sti();
      }


    int ret = current_thread()->handle_page_fault(pfa, error_code, pc, ret_frame);
    if (!ret)
    {
		printf("pfa=0x%08lx pc=0x%08lx\n", pfa, pc);
      panic("Couldn't resolve page fault!\n");
    }

    return ret;
  }

  void slowtrap_entry(Trap_state * /*ts*/)
  {
    NOT_IMPL_PANIC;
  }

};

IMPLEMENT inline
bool
Thread::pagein_tcb_request(Return_frame * /*regs*/)
{
  NOT_IMPL_PANIC;
  return false;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

/** Constructor.
    @param id user-visible thread ID of the sender
    @param init_prio initial priority
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread()
  : Sender            (0),	// select optimized version of constructor
    _pager(Thread_ptr::Invalid),
    _exc_handler(Thread_ptr::Invalid),
    _del_observer(0)
{

  assert(state(false) == Thread_invalid);

  inc_ref();
  _space.space(Kernel_task::kernel_task());

  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;
  _recover_jmpbuf = 0;
  _timeout = 0;

  // reserve an initial stack frame 
  _kernel_sp = (Mword*)((Mword)_kernel_sp - Config::Stack_frame_size);
  // preserve stack alignment
  --_kernel_sp;
  // set the initial restart address of the thread
  // (stored at $sp-2words, cp. switch_cpu())
  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);
  r->psr = 0;

  state_add_dirty(Thread_dead, false);
  // ok, we're ready to go!
}

IMPLEMENT inline
Mword
Thread::user_sp() const
{ return regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{ return regs()->sp(sp); }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
Mword
Thread::user_ip() const
{ return exception_triggered() ? _exc_cont.ip() : regs()->ip(); }

IMPLEMENT inline
Mword
Thread::user_flags() const
{ return 0; }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
void
Thread::user_ip(Mword ip)
{
  if (exception_triggered())
    _exc_cont.ip(ip);
  else
    {
      Entry_frame *r = regs();
      r->ip(ip);
    }
}

PUBLIC inline NEEDS ["trap_state.h"]
int
Thread::send_exception_arch(Trap_state * /*ts*/)
{
  NOT_IMPL_PANIC;
  return 1;      // We did it
}

PROTECTED inline
void
Thread::vcpu_resume_user_arch()
{}


PRIVATE static inline
void
Thread::save_fpu_state_to_utcb(Trap_state *, Utcb *)
{
}

PROTECTED inline
int
Thread::do_trigger_exception(Entry_frame * /*r*/, void * /*ret_handler*/)
{
  NOT_IMPL_PANIC;
  return 0;
}

PRIVATE static inline
bool FIASCO_WARN_RESULT
Thread::copy_utcb_to_ts(L4_msg_tag const &/*tag*/, Thread * /*snd*/,
                        Thread * /*rcv*/, L4_fpage::Rights /*rights*/)
{
  NOT_IMPL_PANIC;
  return true;
}

PRIVATE static inline
bool FIASCO_WARN_RESULT
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread * /*snd*/, Thread * /*rcv*/,
                        L4_fpage::Rights /*rights*/)
{
  NOT_IMPL_PANIC;
  return true;
}

PROTECTED inline
L4_msg_tag
Thread::invoke_arch(L4_msg_tag /*tag*/, Utcb * /*utcb*/)
{
  return commit_result(-L4_err::ENosys);
}

PROTECTED inline
int
Thread::sys_control_arch(Utcb *)
{
  return 0;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

PUBLIC static inline
bool
Thread::check_for_ipi(unsigned)
{ return false; }
