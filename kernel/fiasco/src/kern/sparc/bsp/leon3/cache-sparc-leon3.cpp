#include "processor.h"
#include <cstdio>

INTERFACE [sparc && leon3]:

EXTENSION class Cache
{
    enum Asi
    {
      Force_miss   = 0x01,
      Registers    = 0x02,
      Icache_tags  = 0x0C,
      Icache_data  = 0x0D,
      Dcache_tags  = 0x0E,
      Dcache_data  = 0x0F,
      Flush_inst   = 0x10,
      Flush_data   = 0x11,
    };

    enum Registers
    {
      Cache_control_register  = 0x00,
      Reserved                = 0x04,
      Icache_config_register  = 0x08,
      Dcache_config_register  = 0x0C,
    };

    enum Config_register_bits
    {
      Config_cache_locking  = 31,
      Config_cache_snooping = 27,
      Config_local_ram      = 19,
      Config_mmu            = 3,
    };

    enum Control_register_bits
    {
      Control_dsnoop_enable       = 23,
      Control_flush_data          = 22,
      Control_flush_inst          = 21,
    };
};

IMPLEMENTATION [sparc && leon3]:

IMPLEMENT inline
void
Cache::flush_caches()
{
  asm volatile("flush");
}

IMPLEMENT inline
void
Cache::flush_dcache()
{
  Proc::write_alternative<Flush_data>(0xDEADBEEF, 0);
}

IMPLEMENT inline
void
Cache::flush_icache()
{
  // remark: also flushed dcache if MMU is present
  Proc::write_alternative<Flush_inst>(0xDEADBEEF, 0);
}

IMPLEMENTATION [sparc && leon3 && !debug]:

IMPLEMENT inline
void
Cache::init()
{
}

IMPLEMENTATION [sparc && leon3 && debug]:

IMPLEMENT inline
void
Cache::init()
{
  // dump registers
  Mword ccr = Proc::read_alternative<Registers>(Cache_control_register);
  printf("Cache Control Register:  0x%08lx\n", ccr);
  Mword dcr = Proc::read_alternative<Registers>(Dcache_config_register);
  printf("D-Cache Config Register: 0x%08lx\n", dcr);
  Mword icr = Proc::read_alternative<Registers>(Icache_config_register);
  printf("I-Cache Config Register: 0x%08lx\n", icr);
}
