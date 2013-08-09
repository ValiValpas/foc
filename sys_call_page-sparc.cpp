INTERFACE:

#include "types.h"

//------------------------------------------------------------------------------
IMPLEMENTATION:

#include "mem_layout.h"
#include "kernel_task.h"
#include "mem_space.h"
#include "vmem_alloc.h"

IMPLEMENT static
void
Sys_call_page::init()
{
  Mword *sys_calls = (Mword*)Mem_layout::Syscalls;
  if (!Vmem_alloc::page_alloc(sys_calls,
                              Vmem_alloc::NO_ZERO_FILL, Vmem_alloc::User))
    panic("FIASCO: can't allocate system-call page.\n");

  for (unsigned i = 0; i < Config::PAGE_SIZE - 4*sizeof(Mword); i += sizeof(Mword))
    *(sys_calls++) = 0x91d02000; // ta 0

  *(sys_calls+0) = 0x91d02010; // mem op: ta 0x10
  *(sys_calls+1) = 0x91d02010; // invoke: ta 0x10
  *(sys_calls+2) = 0x91d02000;
  *(sys_calls+3) = 0x91d02000;

  Kernel_task::kernel_task()
    ->set_attributes(Virt_addr(Mem_layout::Syscalls),
	             Page::Attr(Page::Rights::URX(), Page::Type::Normal(),
		                Page::Kern::Global()));
}
