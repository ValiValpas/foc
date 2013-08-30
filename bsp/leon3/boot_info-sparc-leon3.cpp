IMPLEMENTATION[sparc && leon3]:

#include<sparc_types.h>
#include "panic.h"
#include "processor.h"
#include "mem_unit.h"
#include <cassert>

/// interface for sparc leon3 plug and play section (see grlib.pdf)
class Pnp
{
  enum Leon3_pnp {
    // identification register:
    Ahb_master_start = 0xFFFFF000,
    Ahb_master_num   = 64,
    Ahb_slave_start  = 0xFFFFF800,
    Ahb_slave_num    = 64,
    Apb_start        = 0x000FF000,
    Apb_num          = 512,
    Vendor_id_mask   = 0xFF,
    Vendor_id_shift  = 24,
    Device_id_mask   = 0xFFF,
    Device_id_shift  = 12,
    Version_mask     = 0x1F,
    Version_shift    = 5,
    Irq_mask         = 0xF,
    Irq_shift        = 0,
    // bar registers:
    Addr_mask        = 0xFFF,
    Addr_shift       = 20,
    Prefetch_mask    = 0x1,
    Prefetch_shift   = 17,
    Cacheable_mask   = 0x1,
    Cacheable_shift  = 16,
    Mask_mask        = 0xFFF,
    Mask_shift       = 4,
    Type_mask        = 0xF,
    Type_shift       = 0,
    Type_apb_io      = 0x1,
    Type_ahb_mem     = 0x2,
    Type_ahb_io      = 0x3,
    Addr_ahb_shift   = 20,
    Addr_apb_shift   = 8,
  };

  static Address _apb_base;

  public:
    enum Vendor_ids {
      Gaisler    = 0x01,
      Esa        = 0x04,
    };

    // cp. grip.pdf
    enum Gaisler_Devices {
      APBUART    = 0x00C,
      AHBBRIDGE  = 0x020,
      AHBRAM     = 0x00E,
      AHBDPRAM   = 0x00F,
      AHBROM     = 0x01B,
      AHBSTAT    = 0x052,
      AHBUART    = 0x007,
      APBCTRL    = 0x006,
      IRQMP      = 0x00D,
      GRGPIO     = 0x01A,
      GPTIMER    = 0x011,
    };

    struct Ahb_info_record {
      Mword id_reg;
      Mword user1;
      Mword user2;
      Mword user3;
      Mword bar0;
      Mword bar1;
      Mword bar2;
      Mword bar3;
    };

    struct Apb_info_record {
      Mword id_reg;
      Mword bar;
    };

    static inline bool check_ahb_device(Vendor_ids vid, Mword device, struct Ahb_info_record *ahb_info)
    {
        Mword id_reg = Proc::read_alternative<Mmu::Bypass>((Mword)&ahb_info->id_reg);

        Vendor_ids cur_vid = (Vendor_ids)((id_reg >> Vendor_id_shift) & Vendor_id_mask);
        Mword      cur_dev = (id_reg >> Device_id_shift) & Device_id_mask;
        if (cur_vid == vid && cur_dev == device) {
          return true;
        }
        return false;
    }

    static inline bool check_apb_device(Vendor_ids vid, Mword device, struct Apb_info_record *apb_info)
    {
        Mword id_reg = Proc::read_alternative<Mmu::Bypass>((Mword)&apb_info->id_reg);

        Vendor_ids cur_vid = (Vendor_ids)((id_reg >> Vendor_id_shift) & Vendor_id_mask);
        Mword      cur_dev = (id_reg >> Device_id_shift) & Device_id_mask;
        if (cur_vid == vid && cur_dev == device) {
          return true;
        }
        return false;
    }

    static struct Ahb_info_record *find_ahb_device(Vendor_ids vid, Mword device)
    {
      // iterate AHB master records
      struct Ahb_info_record *ahb_info = (struct Ahb_info_record *)Ahb_master_start;
      for (unsigned i=0; i < Ahb_master_num; i++) {
        if (check_ahb_device(vid, device, ahb_info)) {
          return ahb_info;
        }
        ahb_info++;
      }

      ahb_info = (struct Ahb_info_record *)Ahb_slave_start;
      for (unsigned i=0; i < Ahb_slave_num; i++) {
        if (check_ahb_device(vid, device, ahb_info)) {
          return ahb_info;
        }
        ahb_info++;
      }

      return 0UL;
    }
    
    static Address find_ahb_device_address(Vendor_ids vid, Mword device)
    {
      struct Ahb_info_record *ahb_info = find_ahb_device(vid, device);

      if (ahb_info != 0UL) {
        // TODO select correct BAR register
        Mword bar = Proc::read_alternative<Mmu::Bypass>((Mword)&ahb_info->bar0);
        if (bar) {
          return ((bar >> Addr_shift) & Addr_mask) << Addr_ahb_shift;
        }
      }

      return 0UL;
    }

    static inline void init_apb_base()
    {
      if (!_apb_base) 
        _apb_base = find_ahb_device_address(Pnp::Vendor_ids::Gaisler, Pnp::Gaisler_Devices::APBCTRL);

      assert(_apb_base);
    }

    static Apb_info_record *find_apb_device(Vendor_ids vid, Mword device)
    {
      init_apb_base();

      struct Apb_info_record *apb_info = (struct Apb_info_record *)(_apb_base | Apb_start);
      for (unsigned i=0; i < Apb_num; i++) {
        if (check_apb_device(vid, device, apb_info)) {
          return apb_info;
        }
        apb_info++;
      }

      return 0UL;
    }
    
    static Address find_apb_device_address(Vendor_ids vid, Mword device)
    {
      struct Apb_info_record *apb_info = find_apb_device(vid, device);

      if (apb_info != 0UL) {
        Mword bar = Proc::read_alternative<Mmu::Bypass>((Mword)&apb_info->bar);
        if (bar) {
          return (((bar >> Addr_shift) & Addr_mask) << Addr_apb_shift) | _apb_base;
        }
      }

      return 0UL;
    }

    static Address find_device_address(Vendor_ids vid, Mword device)
    {
      Address addr = find_ahb_device_address(vid, device);
      if (addr)
        return addr;

      addr = find_apb_device_address(vid, device);
      return addr;
    }
};

Address
Pnp::_apb_base = 0;

//------------------------------------------------------------------------
// Boot_info

Address
Boot_info::_pic_base = 0;

IMPLEMENT static
Address Boot_info::pic_base()
{
  return _pic_base;
}

PUBLIC static
Address Boot_info::mbar()
{
  panic("mbar address not implemented\n");
  return ~0;
}

IMPLEMENT static
void
Boot_info::lookup_devices()
{
  _pic_base = Pnp::find_device_address(Pnp::Vendor_ids::Gaisler, Pnp::Gaisler_Devices::IRQMP);

  // we map 0x8... to 0xE... in sparc/paging.cpp
  assert((_pic_base & 0xF0000000) == 0x80000000);
  _pic_base |= 0xE0000000;
}

IMPLEMENTATION [sparc && debug]:

#include <cstdio>

PUBLIC static
void Boot_info::dump()
{
  printf("Boot info dump:\n");
  Address addr = Pnp::find_ahb_device_address(Pnp::Vendor_ids::Gaisler, Pnp::Gaisler_Devices::APBCTRL);
  printf("AHB/APB Bridge at 0x%08lx\n", addr);

  addr = Pnp::find_device_address(Pnp::Vendor_ids::Gaisler, Pnp::Gaisler_Devices::APBUART);
  printf("APB UART at 0x%08lx\n", addr);

  addr = Pnp::find_device_address(Pnp::Vendor_ids::Gaisler, Pnp::Gaisler_Devices::IRQMP);
  printf("IRQMP at 0x%08lx\n", addr);
}
