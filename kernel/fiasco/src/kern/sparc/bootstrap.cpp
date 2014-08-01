INTERFACE [sparc]:
#include <cstddef>
#include "types.h"
#include "mem_layout.h"

IMPLEMENTATION [sparc]:
#include "cpu.h"

extern Mword context_table[16];
extern Mword kernel_srmmu_l1[256];

namespace Bootstrap {

struct Order_t;
struct Phys_addr_t;
struct Virt_addr_t;

typedef cxx::int_type<unsigned, Order_t> Order;
typedef cxx::int_type_order<Unsigned32, Virt_addr_t, Order> Virt_addr;

}

IMPLEMENTATION [sparc]:

#include "paging.h"
#include "mem_unit.h"
#include "mem.h"

namespace Bootstrap {
typedef cxx::int_type_order<Unsigned32, Phys_addr_t, Order> Phys_addr;
inline Order map_page_order() { return Order(24); }

inline Mword
pt_entry(Phys_addr pa, bool cache)
{
  Mword res = cxx::int_value<Phys_addr>(cxx::mask_lsb(pa, map_page_order()));
  res = res >> Pte_ptr::Ptp_addr_shift;

  if (cache)
    res |= Pte_ptr::Cacheable;

  res |= Pte_ptr::Accperm_NO_RWX << Pte_ptr::Accperm_shift;

  res |= Pte_ptr::ET_pte;
  return res;
}

static void
map_memory(void volatile *pd, Virt_addr va, Phys_addr pa,
           bool cache)
{
  Mword *const p = (Mword*)pd;
  p[cxx::int_value<Virt_addr>(va >> map_page_order())] = pt_entry(pa, cache);
}

}

asm
(
".section .text.init, \"ax\"          \n"
".global _start                       \n"
"_start:                              \n"
"  ba bootstrap_main                  \n"
"  nop                                \n"
"  ta 0                               \n"
"1:                                   \n"
" ba 1b                               \n"
".previous                            \n"
);

extern "C" void _start_kernel(void);

extern "C"
void bootstrap_main()
{
  // REMARK: DON'T CALL ANY FUNCTION HERE (only inline)
  // since we have neither set the sp nor have we initialised
  // the trap base register up to this point
  typedef Bootstrap::Phys_addr Phys_addr;
  typedef Bootstrap::Virt_addr Virt_addr;
  typedef Bootstrap::Order Order;

  Mword  phys_offset        = Mem_layout::Map_base - Mem_layout::Ram_phys_base;
  Mword *context_table_phys = (Mword*)((Mword)context_table - phys_offset);
  Mword *page_dir_phys      = (Mword*)((Mword)kernel_srmmu_l1 - phys_offset);
 
  // clear context table
  Mem::memset_mwords(context_table_phys, 0, sizeof(context_table));

  // clear 1st-level PD
  Mem::memset_mwords(page_dir_phys, 0, sizeof(kernel_srmmu_l1));

  // set context table pointer and context number
  Mem_unit::context_table(Address(context_table_phys));
  Mem_unit::context(0);

  /* add context table entry for 1st-level PD */
  Mword shifted = (Mword)page_dir_phys >> Pte_ptr::Ptp_addr_shift;
  context_table_phys[0] = shifted | Pte_ptr::ET_ptd;

  Virt_addr va;
  Phys_addr pa;

  /* map phys mem starting from VA 0xF0000000 */
  va = Virt_addr(Mem_layout::Map_base);
  pa = Phys_addr(Mem_layout::Ram_phys_base);
  Bootstrap::map_memory(page_dir_phys, va, pa, true);
  
  /* map phys mem 1:1 as well */
  va = Virt_addr(Mem_layout::Ram_phys_base);
  pa = Phys_addr(Mem_layout::Ram_phys_base);
  Bootstrap::map_memory(page_dir_phys, va, pa, true);

  /* map apb */
  va = Virt_addr(Mem_layout::Uart_base);
  pa = Phys_addr(Mem_layout::Uart_phys_base);
  Bootstrap::map_memory(page_dir_phys, va, pa, false);

  Mem_unit::mmu_enable();
  asm volatile("nop; nop; nop;");

  _start_kernel();

  while(1);
}
