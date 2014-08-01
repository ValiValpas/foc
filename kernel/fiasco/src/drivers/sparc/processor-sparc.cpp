#include "types.h"
#include "psr.h"
#include "processor.h"
#include "warn.h"

IMPLEMENTATION [sparc]:

EXTENSION class Proc
{
public:
  //disable power savings mode
 static Mword wake(Mword);
 static unsigned cpu_id();
 static Mword frame_pointer();
 static void  frame_pointer(Mword);
 static Mword return_address();
 static void  return_address(Mword);
 static Mword wim();
 static void  flush_regwins();
};

/// Unblock external interrupts
IMPLEMENT static inline
void Proc::sti()
{
  unsigned p = Psr::read();
  p &= ~(0xF << Psr::Interrupt_lvl);
  Psr::write(p);
}

/// Block external interrupts
IMPLEMENT static inline
void Proc::cli()
{
  unsigned p = Psr::read();
  p |= (0xF << Psr::Interrupt_lvl);
  Psr::write(p);
}

/// Are external interrupts enabled ?
IMPLEMENT static inline
Proc::Status Proc::interrupts()
{
  unsigned mask = 0xF << Psr::Interrupt_lvl;
  unsigned psr = Psr::read();
  if (!(psr & 1 << Psr::Enable_trap))
    return false;
  
  // only return false if all interrupts are disabled
  return (Psr::read() & mask) != mask;
}

/// Block external interrupts and save the old state
IMPLEMENT static inline
Proc::Status Proc::cli_save()
{
  Status ret = Psr::read() & (0xF << Psr::Interrupt_lvl);
  Proc::cli();
  return ret;
}

/// Conditionally unblock external interrupts
IMPLEMENT static inline
void Proc::sti_restore(Status status)
{
  Psr::write(status);
}

IMPLEMENT static inline
void Proc::pause()
{
}

IMPLEMENT static inline
void Proc::halt()
{
  // this should enable some kind of power saving on the LEON3 (not tested)
  asm volatile ("wr %g0, %asr19");
}

IMPLEMENT static inline
Mword Proc::wake(Mword srr1)
{
  (void)srr1;
  return 0; // XXX
}

IMPLEMENT static inline
void Proc::irq_chance()
{
  // XXX?
  asm volatile ("nop; nop;" : : :  "memory");
}

IMPLEMENT static inline
void Proc::stack_pointer(Mword sp)
{
  (void)sp;
  asm volatile ("mov %0, %%sp\n" : : "r"(sp));
}

IMPLEMENT static inline
Mword Proc::stack_pointer()
{
  Mword sp = 0;
  asm volatile ("mov %%sp, %0\n" : "=r" (sp));
  return sp;
}

IMPLEMENT static inline
void Proc::return_address(Mword i7)
{
  (void)i7;
  asm volatile ("mov %0, %%i7\n" : : "r"(i7));
}

IMPLEMENT static inline
Mword Proc::return_address()
{
  Mword i7 = 0;
  asm volatile ("mov %%i7, %0\n" : "=r" (i7));
  return i7;
}

IMPLEMENT static inline
Mword Proc::frame_pointer()
{
  Mword fp = 0;
  asm volatile ("mov %%fp, %0\n" : "=r" (fp));
  return fp;
}

IMPLEMENT static inline
void Proc::frame_pointer(Mword fp)
{
  (void)fp;
  asm volatile ("mov %0, %%fp\n" : : "r"(fp));
}

IMPLEMENT static inline
Mword Proc::wim()
{
  Mword wim = 0;
  asm volatile ("mov %%wim, %0\n" : "=r" (wim));
  return wim;
}

IMPLEMENT static inline
Mword Proc::program_counter()
{
  Mword pc = 0;
  asm volatile ("call 1\n\t"
                "nop\n\t" // delay instruction
		"1: mov %%o7, %0\n\t"
		: "=r" (pc) : : "o7");
  return pc;
}

PUBLIC static inline
template <int ASI>
Mword Proc::read_alternative(Mword reg)
{
	Mword ret;
	asm volatile("lda [%1] %2, %0"
				  : "=r" (ret)
				  : "r" (reg),
				    "i" (ASI));
	return ret;

}

PUBLIC static inline
template <int ASI>
void Proc::write_alternative(Mword reg, Mword value)
{
	asm volatile ("sta %0, [%1] %2\n\t"
				  :
				  : "r"(value),
				    "r"(reg),
				    "i"(ASI));
}

IMPLEMENT static inline
void Proc::flush_regwins()
{
  const unsigned depth = Config::Num_register_windows - 1;

  for (unsigned i = 0; i < depth; i++)
    asm volatile("save");

  for (unsigned i = 0; i < depth; i++)
    asm volatile("restore");
}

IMPLEMENTATION [sparc && !mpcore]:

IMPLEMENT static inline
unsigned Proc::cpu_id()
{ return 0; }


