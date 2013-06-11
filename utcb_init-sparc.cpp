IMPLEMENTATION [sparc]:

#include "mem_layout.h"
#include "panic.h"
#include "vmem_alloc.h"
#include "config.h"
#include <cstring>


IMPLEMENT 
void
Utcb_init::init()
{
  //Utcb_ptr_page is physical address
  if (!Vmem_alloc::page_alloc((void *)Mem_layout::Utcb_ptr_page, Vmem_alloc::ZERO_FILL, Vmem_alloc::User))
    panic ("UTCB pointer page allocation failure");
}

PUBLIC static inline
void
Utcb_init::init_ap(Cpu const &)
{}

