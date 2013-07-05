//-------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "mem_layout.h"

IMPLEMENT inline NEEDS["mem_layout.h"]
User<Utcb>::Ptr
Utcb_support::current()
{ return *reinterpret_cast<User<Utcb>::Ptr*>(Mem_layout::Utcb_ptr_page); }

IMPLEMENT inline NEEDS["mem_layout.h"]
void
Utcb_support::current(User<Utcb>::Ptr const &utcb)
{ *reinterpret_cast<User<Utcb>::Ptr*>(Mem_layout::Utcb_ptr_page) = utcb; }
