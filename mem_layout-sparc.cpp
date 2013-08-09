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
    Ram_phys_base        = 0x40000000,   // TODO make this configurable (platform specific)
  };

  enum Virt_layout : Address
  {
    // unmapped addresses
    Utcb_ptr_page        = 0xffffc000,
    Tbuf_status_page     = 0xffffd000,
    Syscalls             = 0xfffff000,
    Tbuf_buffer_area     = 0xffd00000,
    Tbuf_ubuffer_area    = Tbuf_buffer_area,

    User_max             = 0xe0000000,
//    Tcbs                 = 0xc0000000, // TODO this can probably be discarded
    Utcb_addr            = User_max - 0x4000,  ///< UTCB map address, 16kB
    utcb_ptr_align       = Tl_math::Ld<sizeof(void*)>::Res,
//    Tcbs_end             = 0xe0000000, // TODO this can probably be discarded
//    __free_1_start       = 0xec000000, // TODO this can probably be discarded
//    __free_1_end         = 0xed000000, // TODO this can probably be discarded

    // addresses already mapped on startup
    Map_base             = 0xf0000000, ///< 6MB kernel memory
    Map_end              = 0xf0600000,
    Caps_start           = 0xf0600000, ///< 2MB caps
    Caps_end             = 0xf0800000,
    // free mem up to 0xf0a00000?
    Kernel_image         = 0xf0800000, ///< kernel image end
//    Kernel_max           = 0x00000000, // TODO this can probably be discarded
  };
};

//---------------------------------------------------------------------------
INTERFACE [sparc]:

EXTENSION class Mem_layout
{
public:
  enum {
    Uart_phys_base = 0x80000100,
    Uart_base      = 0xe0000100,
  };
};


// ------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "panic.h"
#include "paging.h"

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
