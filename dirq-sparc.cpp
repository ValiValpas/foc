IMPLEMENTATION [sparc]:
// ---------------------------------------------------------------------------
#include "std_macros.h"
#include "timer.h"
#include "processor.h"
#include "pic.h"


IMPLEMENTATION:

#include "irq_chip_generic.h"
#include "irq.h"

extern "C"
void irq_handler()
{
  // FIXME implement irq_handler()
  Return_frame *rf = nonull_static_cast<Return_frame*>(current()->regs());

  printf("irq_handler: l0=0x%08lx\n", rf->l0);
  rf->dump();
//  Mword irq;
//
//  if(EXPECT_TRUE(rf->user_mode()))
//    rf->srr1 = Proc::wake(rf->srr1);
//
//  irq = Pic::pending();
//  if(EXPECT_FALSE(irq == Pic::No_irq_pending))
//    return;
//
//  Irq *i = nonull_static_cast<Irq*>(Pic::main->irq(irq));
//  Irq::log_irq(i, irq);
//  i->hit();
}
