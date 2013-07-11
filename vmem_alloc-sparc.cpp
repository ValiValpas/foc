//---------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "config.h"
#include "panic.h"
#include "warn.h"
#include "mem.h"
#include "mem_space.h"
#include "kmem_alloc.h"

IMPLEMENT
void Vmem_alloc::init()
{
  NOT_IMPL_PANIC;
}

IMPLEMENT
void *Vmem_alloc::page_alloc(void * address, Zero_fill zf, unsigned mode)
{
  // TODO alloc page and map in Vmem_alloc::page_alloc()
  // as for now, we don't need to map a page because we have a 1:1 mapping of physical to
  // virtual adresses

  void *vpage = Kmem_alloc::allocator()->alloc(Config::PAGE_SHIFT);

  if (EXPECT_FALSE(!vpage))
    return 0;

  Address page = Mem_space::kernel_space()->virt_to_phys((Address)vpage);
  assert(page);
  Mem_unit::flush_caches();

  // insert page into master page table
  auto pte = Mem_space::kernel_space()->dir()->walk(Virt_addr(address),
      Pdir::Depth, true, Kmem_alloc::q_allocator(Ram_quota::root));

  if (pte.is_valid()) {
    printf("Inserted page 0x%08lx at level %d, valid=%d\n", (Mword)address, pte.level, pte.is_valid());
    printf("page at 0x%08lx already mapped\n", (Mword)address);
    assert(false);
  }

  Page::Rights r = Page::Rights::RWX();
  if (mode & User)
    r |= Page::Rights::U();

  pte.create_page(Phys_mem_addr(page), Page::Attr(r, Page::Type::Normal(), Page::Kern::Global()));
  pte.write_back_if(true);
  // flush tlb and I/D cache
  Mem_unit::tlb_flush((Address)address);

  if (zf == ZERO_FILL)
    Mem::memset_mwords((unsigned long *)address, 0, Config::PAGE_SIZE >> 2);

  return address;
}

IMPLEMENT
void *Vmem_alloc::page_unmap(void * /*page*/)
{
  NOT_IMPL_PANIC;
  return NULL;
}

