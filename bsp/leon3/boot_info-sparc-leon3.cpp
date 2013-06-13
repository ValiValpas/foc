//FIXME conditional compilation (leon3)
//IMPLEMENTATION[sparc && leon3]:
IMPLEMENTATION[sparc]:

#include<sparc_types.h>
#include "panic.h"

IMPLEMENT static
Address Boot_info::pic_base()
{
  // FIXME query address from AHB/APB plug&play area (see GRLIB User's Manual)
  return 0x80000200;
}

PUBLIC static
Address Boot_info::mbar()
{
  panic("mbar address not implemented\n");
  return ~0;
}
