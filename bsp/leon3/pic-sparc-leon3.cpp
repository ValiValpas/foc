//INTERFACE [sparc && leon3]:
INTERFACE [sparc]:

#include "types.h"
#include "io.h"
#include "boot_info.h"
#include <cassert>

//class Irq_chip_icu;

// FIXME adapt pic to sparc
/**
 * Interface for IRQMP controller (see "GRLIB IP Core User's Manual")
 */
EXTENSION class Pic
{
private:
  ///+0x00: Interrupt Level Register
  static Address level()     { return _pic_base; }
  ///+0x04: Interrupt Pending Register
  static Address stat()      { return _pic_base + 0x04; }
  ///+0x08: Interrupt Force Register (NCPU = 0)
  static Address force()     { return _pic_base + 0x08; }
  ///+0x0C: Interrupt Clear Register
  static Address clear()     { return _pic_base + 0x0C; }
  ///+0x10: Multiprocessor Status Register
  static Address status()    { return _pic_base + 0x10; }
  ///+0x14: Broadcast Register (NCPU > 0)
  static Address broadcast() { return _pic_base + 0x14; }
  ///+0x40: Processor Interrupt Mask Register
  static Address mask()      { return _pic_base + 0x40; }
  ///+0x40 + 4 * n: Processor Interrupt Mask Register Processor N
  static Address mask(unsigned n)
  {
    assert(n < _ncpu);
    return _pic_base + 0x40 + 4*n;
  }
// same as 0x08?
//  ///+0x80: Processor Interrupt Force Register
//  static Address force()      { return _pic_base + 0x80; }
  ///+0x80 + 4 * n: Processor Interrupt Force Register Processor N
  static Address force(unsigned n)
  {
    assert(n < _ncpu);
    return _pic_base + 0x80 + 4*n;
  }
  ///+0xC0: Extended Interrupt Acknowledge Register
  static Address ack()        { return _pic_base + 0xC0; }
  ///+0xC0 + 4 * n: Extended Interrupt Acknowledge Register Processor N
  static Address ack(unsigned n)
  {
    assert(n < _ncpu);
    return _pic_base + 0xC0 + 4*n;
  }


  /** Interrupt  */
  enum Bit_masks
  {
    Irq_mask          = 0x7FFF,
    Irq_shift         = 1,
    Ext_irq_mask      = 0x7FFFFFFF,
    Ext_irq_shift     = 1,
    Ncpu_mask         = 0xF,          // status register
    Ncpu_shift        = 28,           // status register
    Broadcast_shift   = 27,           // status register
    Eirq_number_mask  = 0xF,          // status register
    Eirq_number_shift = 16,           // status register
    Status_mask       = 0xFFFF,       // status register
    Eid_mask          = 0xF,          // extended interrupt ID (acknowledge register)
  };

  static Address _pic_base;
  static unsigned _ncpu;

public:
  enum { Irq_max = 15 };
  enum { No_irq_pending = 0U };

//  static Irq_chip_icu *main;
};

//------------------------------------------------------------------------------
// FIXME conditional compilation
//IMPLEMENTATION [sparc && leon3]:
IMPLEMENTATION [sparc]:

//#include "boot_info.h"
//#include "io.h"
#include "irq.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"
//#include "panic.h"
//#include "sparc_types.h"
//
//#include <cassert>
//#include <cstdio>

//------------------------------------------------------------------------------
// Chip implementation

class Chip : public Irq_chip_gen
{
public:
  Chip() : Irq_chip_gen(Pic::Irq_max) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
};

PUBLIC
void
Chip::mask(Mword pin)
{
  assert(cpu_lock.test());
  Pic::disable_locked(pin);
}

PUBLIC
void
Chip::ack(Mword pin)
{
  assert(cpu_lock.test());
  Pic::acknowledge_locked(pin);
}

PUBLIC
void
Chip::mask_and_ack(Mword pin)
{
  assert(cpu_lock.test());
  Pic::disable_locked(pin);
  Pic::acknowledge_locked(pin);
}

PUBLIC
void
Chip::unmask(Mword pin)
{
  assert(cpu_lock.test());
  Pic::enable_locked(pin);
}

PUBLIC
void
Chip::set_cpu(Mword irq, Cpu_number cpu)
{
  // TODO mask 'irq' for all but 'cpu'?
  (void)irq;
  (void)cpu;
}

PUBLIC
Unsigned32
Chip::pending()
{
  return Pic::pending();
}

//----------------------------------------------------------
// Pic implementation 

static Static_object<Irq_mgr_single_chip<Chip> > mgr;

//Irq_chip_icu *Pic::main;
unsigned Pic::_ncpu;
Address Pic::_pic_base;

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  _ncpu = (status() >> Ncpu_shift) & Ncpu_mask;
  _pic_base = Boot_info::pic_base();

  // clear all
  Io::write<Unsigned32>(0xffffffff, clear());

  Irq_mgr::mgr = mgr.construct();
}

//----------------------------------------------------------
// irq handler
extern "C"
void irq_handler(Mword psr, Mword pc, Mword npc, Mword level)
{
  // FIXME write Return_frame?
//  Return_frame *rf = nonull_static_cast<Return_frame*>(current()->regs());

  printf("psr 0x%08lx\n", psr);
  printf("pc 0x%08lx\n", pc);
  printf("npc 0x%08lx\n", npc);
  printf("level 0x%08lx\n", level);

//  if(EXPECT_TRUE(rf->user_mode()))
//    rf->srr1 = Proc::wake(rf->srr1);

  return; // FIXME do interrupt handling
  mgr->c.handle_multi_pending<Chip>(0);
}

//-------------------------------------------------------------------------------
/**
 * Interface implementation
 */
IMPLEMENT inline
void
Pic::block_locked (unsigned irq_num)
{
  disable_locked(irq_num);
}

IMPLEMENT inline 
void
Pic::acknowledge_locked(unsigned irq_num)
{
  assert(irq_num > 0 && irq_num <= Irq_max);
  Io::write<Unsigned32>(1 << irq_num, clear());
}

IMPLEMENT inline
void
Pic::disable_locked (unsigned irq_num)
{
  assert(irq_num > 0 && irq_num <= Irq_max);
  Io::clear<Unsigned32>(1 << irq_num, mask());
}

IMPLEMENT inline
void
Pic::enable_locked (unsigned irq_num, unsigned /*prio*/)
{
  assert(irq_num > 0 && irq_num <= Irq_max);
  Io::set<Unsigned32>(1 << irq_num, mask());
}

PUBLIC static inline
unsigned
Pic::nr_irqs()
{ return Irq_max; }

PUBLIC static inline
Unsigned32
Pic::pending()
{
  return (Io::read<Unsigned32>(Pic::pending()) >> Pic::Irq_shift) & Pic::Irq_mask;
}

/**
 * disable all interrupts
 */
IMPLEMENT inline
Pic::Status
Pic::disable_all_save()
{
  // TODO disable interrupts for all cpus
  Status s = Io::read<Unsigned32>(mask());
  Io::write<Unsigned32>(0, mask());
  return s;
}

IMPLEMENT inline
void
Pic::restore_all(Status s)
{
  // TODO restore interrupts for all cpus
  Io::write<Unsigned32>(s, mask());
}


// ------------------------------------------------------------------------
IMPLEMENTATION [debug && sparc]:

PUBLIC
char const *
Chip::chip_type() const
{ return "Sparc IRQMP"; }
