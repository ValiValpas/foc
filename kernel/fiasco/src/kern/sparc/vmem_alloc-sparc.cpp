//---------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "config.h"
#include "panic.h"
#include "warn.h"
#include "mem.h"
#include "mem_space.h"
#include "kmem_alloc.h"
#include "cache.h"

IMPLEMENT
void Vmem_alloc::init()
{
  NOT_IMPL_PANIC;
}

IMPLEMENT
void *Vmem_alloc::page_alloc(void * address, Zero_fill zf, unsigned mode)
{
  void *vpage = Kmem_alloc::allocator()->alloc(Config::PAGE_SHIFT);

  if (EXPECT_FALSE(!vpage))
    return 0;

  Address page = Mem_space::kernel_space()->virt_to_phys((Address)vpage);
  assert(page);
  Cache::flush_caches();

  // insert page into master page table
  auto pte = Mem_space::kernel_space()->dir()->walk(Virt_addr(address),
      Pdir::Depth, Kmem_alloc::q_allocator(Ram_quota::root));

  if (pte.e->is_valid()) {
    printf("Inserted page 0x%08lx at level %d, valid=%d\n", (Mword)address, pte.e->level, pte.e->is_valid());
    printf("page at 0x%08lx already mapped\n", (Mword)address);
    assert(false);
  }

  Page::Attribs r = Page::CACHEABLE;
  if (mode & User)
    r = Page::USER_RWX;
  else
    r = Page::KERN_RWX;

  pte.e->create_page(Phys_addr(page), r);
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

