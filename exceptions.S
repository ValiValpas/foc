/* -*- asm -*- */

#include "globalconfig.h"

.text

.macro DEBUGGER_ENTRY
  // TODO create trap_state frame and store ptr in %o0
  call call_nested_trap_handler  
  nop
.endm

.global kern_kdebug_entry
kern_kdebug_entry:
#ifdef CONFIG_JDB
  DEBUGGER_ENTRY
#endif
  ret
  nop

.global vcpu_resume
vcpu_resume:
  ta 0
