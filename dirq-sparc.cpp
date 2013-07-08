IMPLEMENTATION [sparc]:
// ---------------------------------------------------------------------------
#include "std_macros.h"
#include "timer.h"
#include "processor.h"
#include "pic.h"

INTERFACE:
#include "types.h"

IMPLEMENTATION:

#include "irq_chip_generic.h"
#include "irq.h"

extern "C"
void irq_handler(Mword psr, Mword pc, Mword npc, Mword level)
{
  // FIXME implement irq_handler()
  // FIXME write Return_frame?
//  Return_frame *rf = nonull_static_cast<Return_frame*>(current()->regs());

  printf("psr 0x%08lx\n", psr);
  printf("pc 0x%08lx\n", pc);
  printf("npc 0x%08lx\n", npc);
  printf("level 0x%08lx\n", level);

//  if(EXPECT_TRUE(rf->user_mode()))
//    rf->srr1 = Proc::wake(rf->srr1);
//
//  irq = Pic::pending();
//  if(EXPECT_FALSE(irq == Pic::No_irq_pending))
//    return;

  handle_multi_pending<Irq_chip_icu>(0);
}
