//---------------------------------------------------------------------------
INTERFACE [sparc]:

#include "initcalls.h"
#include "template_math.h"
#include <cstdio>

EXTENSION class Mem_layout
{

//TODO cbass: check what can be omitted
public:
  enum Phys_layout : Address
  {
    // remark: all above 0x40e0000 seems to be free
    Utcb_ptr_page        = 0x40ff3000,
    Syscalls_phys        = 0x40ff4000,
    Tbuf_status_page     = 0x40ff5000,
    Kernel_start         = 0x40ff6000,
  };

  enum Virt_layout : Address
  {
    Syscalls             = 0xfffff000,   // FIXME check

    User_max             = 0xf0000000,
//    Tcbs                 = 0xc0000000, // TODO this can probably be discarded
    Utcb_addr            = User_max - 0x2000,  ///< UTCB map address, 8kB
    utcb_ptr_align       = Tl_math::Ld<sizeof(void*)>::Res,
//    Tcbs_end             = 0xe0000000, // TODO this can probably be discarded
//    __free_1_start       = 0xec000000, // TODO this can probably be discarded
//    __free_1_end         = 0xed000000, // TODO this can probably be discarded
    Map_base             = 0xf0000000, ///< 4MB kernel memory
    Map_end              = 0xf0400000,
    Caps_start           = 0xf0400000, ///< 2MB caps
    Caps_end             = 0xf0600000,
    // free mem up to 0xf0a00000?
    Tbuf_start           = 0xf0600000, // FIXME alloc tbuf memory instead (but from where?)
    Kernel_image         = 0xf0800000, ///< kernel image end
//    Kernel_max           = 0x00000000, // TODO this can probably be discarded
  };

  static Address Tbuf_buffer_area;
  static Address Tbuf_ubuffer_area;
};

//---------------------------------------------------------------------------
INTERFACE [sparc]:

EXTENSION class Mem_layout
{
public:
  enum {
    Uart_base = 0x80000100,
  };
};


// ------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "panic.h"
#include "paging.h"

Address Mem_layout::Tbuf_buffer_area = 0;
Address Mem_layout::Tbuf_ubuffer_area = 0;

PUBLIC static
Address
Mem_layout::phys_to_pmem (Paddress addr)
{
  // TODO move kernel_srmmu_l1 to mem_layout (cp arm/mem_layout-noncont.cpp)
  extern Mword kernel_srmmu_l1[256];
  for (unsigned i = 0xF0; i < 0xFF; ++i)
    {
      // find addr in PD
      if (kernel_srmmu_l1[i] != 0)
        {
          Pte_ptr ppte(&kernel_srmmu_l1[i], 0);
          unsigned page_order = ppte.page_order();
          Dword entry  = ppte.page_addr();
          Dword page   = cxx::mask_lsb(addr, page_order);
          if (entry == page) {
            // return virtual address
            return (i << page_order) | cxx::get_lsb(addr, page_order);
          }
        }
    }
  return Invalid_address;
}

PUBLIC static inline
Paddress
Mem_layout::pmem_to_phys (Address addr)
{
  // FIXME implement PT walk
  printf("Mem_layout::pmem_to_phys(Address addr=%lx) is not implemented\n", addr);
  return Invalid_paddress;
}

PUBLIC static inline
Paddress
Mem_layout::pmem_to_phys (const void *ptr)
{
  return pmem_to_phys(reinterpret_cast<Address>(ptr));
}

PUBLIC static inline
template< typename V >
bool
Mem_layout::read_special_safe(V const * /* *address */, V &/*v*/)
{
  panic("%s not implemented", __PRETTY_FUNCTION__);
  return false;
}

PUBLIC static inline
template< typename T >
T
Mem_layout::read_special_safe(T const *a)
{
  Mword res;
  return T(res);
}


/* no page faults can occur, return true */
PUBLIC static inline
bool
Mem_layout::is_special_mapped(void const * /*a*/)
{
  return true;
}


IMPLEMENTATION [sparc && debug]:

#include "kip_init.h"


PUBLIC static FIASCO_INIT
void
Mem_layout::init()
{
  Mword alloc_size = 0x200000;
  unsigned long max = ~0UL;
  for (;;)
    {
      Mem_region r;
      r.start=Tbuf_start;
      r.end=Kernel_image-1;
      if (r.start > r.end)
        panic("Corrupt memory descscriptor in KIP...");

      if (r.start == r.end)
        panic("not enough kernel memory");

      max = r.start;
      Mword size = r.end - r.start + 1;
      if(alloc_size <= size)
        {
          r.start += (size - alloc_size);
          Kip::k()->add_mem_region(Mem_desc(r.start, r.end,
                                            Mem_desc::Reserved));

          printf("TBuf  installed at: [%08lx; %08lx] - %lu KB\n", 
                 r.start, r.end, alloc_size / 1024);

          Tbuf_buffer_area = Tbuf_ubuffer_area = r.start;
          break;
        }
    }

    if(!Tbuf_buffer_area)
      panic("Could not allocate trace buffer");
}

IMPLEMENTATION [sparc && !debug]:

PUBLIC static FIASCO_INIT
void
Mem_layout::init()
{}
