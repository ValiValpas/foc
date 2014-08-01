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

  // user invoke code (see thread-sparc.cpp)
  *(sys_calls++) = 0x818bc000; // wrpsr %o7
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x81c04000; // jmp %g1
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop
  *(sys_calls++) = 0x01000000; // nop

  for (unsigned i = 10*sizeof(Mword); i < Config::PAGE_SIZE - 10*sizeof(Mword); i += sizeof(Mword))
    *(sys_calls++) = 0x91d02000; // ta 0

  // write 10 entries that will be resolved by the sys_call_table
  *(sys_calls+0) = 0x91d02010;
  *(sys_calls+1) = 0x91d02010;
  *(sys_calls+2) = 0x91d02010;
  *(sys_calls+3) = 0x91d02010;
  *(sys_calls+4) = 0x91d02010;
  *(sys_calls+5) = 0x91d02010;
  *(sys_calls+6) = 0x91d02010;
  *(sys_calls+7) = 0x91d02010; // ipc invoke
  *(sys_calls+8) = 0x91d02010;
  *(sys_calls+9) = 0x91d02010;

  Kernel_task::kernel_task()
    ->set_attributes(Virt_addr(Mem_layout::Syscalls),
	             Page::USER_RX | Page::CACHEABLE);
}
