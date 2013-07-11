INTERFACE [sparc]:

#include "types.h"
#include "processor.h"

namespace Mmu
{
    enum ASI
    {
      Cache_miss    = 0x01,
      Sys_control   = 0x02,
      Icache_tags   = 0x0C,
      Icache_data   = 0x0D,
      Dcache_tags   = 0x0E,
      Dcache_data   = 0x0F,
      Icache_flush  = 0X10,
      Dcache_flush  = 0x11,
      Flush_context = 0x13,
      Diag_dcache   = 0x14,
      Diag_icache   = 0x15,
      Flush         = 0x18,
      Regs          = 0x19,
      Bypass        = 0x1C,
      Diagnostic    = 0x1D,
      Snoop_diag    = 0x1E
    };

    enum Registers
    {
      Control       = 0x000,
      ContextTable  = 0x100,
      ContextNumber = 0x200,
    };

    enum Flush_types
    {
      Page     = 0x000,
      Segment  = 0x100,
      Region   = 0x200,
      Context  = 0x300,
      Entire   = 0x400,
    };
};

class Mem_unit { };

//------------------------------------------------------------------------------
IMPLEMENTATION[sparc && !mp]:

/**
 * Flush entire TLB
 */
PUBLIC static inline
void
Mem_unit::tlb_flush()
{
  Proc::write_alternative<Mmu::Flush>(0, Mmu::Flush_types::Entire);
}

/**
 * Flush context
 */
PUBLIC static inline
void
Mem_unit::tlb_flush_context()
{
  Proc::write_alternative<Mmu::Flush>(0, Mmu::Flush_types::Context);
}

/**
 * Flush page at virtual address
 */
PUBLIC static inline
void 
Mem_unit::tlb_flush(Address addr)
{
  // Flush from level 1 downwards
  Proc::write_alternative<Mmu::Flush>(0, Mmu::Flush_types::Region | (addr & 0xFFFFF000));
}

PUBLIC static inline
void
Mem_unit::sync()
{
}

PUBLIC static inline
void
Mem_unit::isync()
{
}

PUBLIC static inline
void 
Mem_unit::flush_caches()
{
  asm volatile ("flush");
}

PUBLIC static inline
void
Mem_unit::context(Mword number)
{
  Proc::write_alternative<Mmu::Regs>(Mmu::ContextNumber, number);
}

PUBLIC static inline
Mword
Mem_unit::context()
{
  return Proc::read_alternative<Mmu::Regs>(Mmu::ContextNumber);
}

PUBLIC static inline
void
Mem_unit::context_table(Address table)
{
  Proc::write_alternative<Mmu::Regs>(Mmu::ContextTable, (table >> 4) & ~0x3);
}

PUBLIC static inline
void
Mem_unit::mmu_enable()
{
  Mword r = Proc::read_alternative<Mmu::Regs>(Mmu::Control);
  r |= 1;
  Proc::write_alternative<Mmu::Regs>(Mmu::Control, r);
}
