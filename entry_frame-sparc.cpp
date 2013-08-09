INTERFACE[sparc]:

#include "types.h"
#include "warn.h"

EXTENSION class Syscall_frame
{
  public:
    Mword i0; // tag
    Mword i1; // dest
    Mword i2; // timeout
    Mword i3; // label
    Mword i4;
    Mword i5;
    Mword i6;
    Mword i7;
    void dump() const;
};

class Stack_frame
{
  // as in kern/sparc/crt0.S
  public:
    Mword l0;   //0
    Mword l1;   //+4
    Mword l2;   //+8
    Mword l3;   //+12
    Mword l4;   //+16
    Mword l5;   //+20
    Mword l6;   //+24
    Mword l7;   //+28
    Mword i0;   //+32
    Mword i1;   //+36
    Mword i2;   //+40
    Mword i3;   //+44
    Mword i4;   //+48
    Mword i5;   //+52
    Mword i6;   //+56
    Mword i7;   //+60
    Mword reserved[8]; //+64 to +92
};

EXTENSION class Return_frame
{
  // as in kern/sparc/crt0.S
  public:
    Mword usp;
    // FIXME some confusion about user return address and the purpose of spill/fill_user_state
    Mword ura; // user return address (i7), dont think we need to save this on sparc
    Mword pc;
    Mword psr;
    void dump();
    void dump_scratch();
    bool user_mode();
};

//------------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "warn.h"
#include "psr.h"

IMPLEMENT
void
Syscall_frame::dump() const
{
  printf("Sparc syscall_frame is empty.\n");
//  printf("IP: %08lx\n", r[29]);
//  printf("R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx\n",
//          0, r[0], 2, r[1], 3, r[2], 4, r[3], 5, r[4]);
//  printf("R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx R[%2d]: %08lx\n",
//          6, r[5], 7, r[6], 8, r[7], 9, r[8], 10, r[9]);
}

//PRIVATE
//void
//Return_frame::psr_bit_scan()
//{
//  printf("SRR1 bits:");
//  for(int i = 31; i >= 0; i--)
//    if(srr1 & (1 << i))
//     printf(" %d", 31-i);
//  printf("\n");
//}

IMPLEMENT
void
Return_frame::dump()
{
//  printf("L0: %08lx L1: %08lx L2: %08lx\n"
//         "L3: %08lx L4: %08lx L5: %08lx\n"
//         "L6: %08lx L7: %08lx\n"
//         "I0: %08lx I1: %08lx I2: %08lx\n"
//         "I3: %08lx I4: %08lx I5: %08lx\n"
//         "I6: %08lx I7: %08lx\n",
//         l0, l1, l2, l3, l4, l5, l6, l7,
//         i0, i1, i2, i3, i4, i5, i6, i7);
//  psr_bit_scan();
}
//
//IMPLEMENT
//void
//Return_frame::dump_scratch()
//{
//  printf("\nR[%2d]: %08lx\nR[%2d]: %08lx\n", 11, r11, 12, r12);
//}
//
IMPLEMENT inline
Mword
Return_frame::sp() const
{
  return Return_frame::usp;
}

IMPLEMENT inline
void
Return_frame::sp(Mword _sp)
{
  Return_frame::usp = _sp;
}

IMPLEMENT inline
Mword
Return_frame::ip() const
{
  return Return_frame::pc;
}

IMPLEMENT inline
void
Return_frame::ip(Mword _pc)
{
  Return_frame::pc = _pc;
}

//IMPLEMENT inline NEEDS ["psr.h"]
//bool
//Return_frame::user_mode()
//{
//  return 0;
//  /*return Msr::Msr_pr & srr1;*/
//}

//---------------------------------------------------------------------------
//TODO cbass: set registers properly 
IMPLEMENT inline
Mword Syscall_frame::next_period() const
{ return false; }

IMPLEMENT inline
void Syscall_frame::from(Mword id)
{
  i3 = id;
}

IMPLEMENT inline
Mword Syscall_frame::from_spec() const
{
  return i3;
}


IMPLEMENT inline
L4_obj_ref Syscall_frame::ref() const
{
  return L4_obj_ref::from_raw(i1);
}

IMPLEMENT inline
void Syscall_frame::ref(L4_obj_ref const &ref)
{
  i1 = ref.raw();
}

IMPLEMENT inline
L4_timeout_pair Syscall_frame::timeout() const
{ 
  return L4_timeout_pair(i2);
}


IMPLEMENT inline 
void Syscall_frame::timeout(L4_timeout_pair const &to)
{
  i2 = to.raw();
}

IMPLEMENT inline Utcb *Syscall_frame::utcb() const
{
  NOT_IMPL_PANIC;
//  return reinterpret_cast<Utcb*>(r[1]); /*r2*/
}

IMPLEMENT inline L4_msg_tag Syscall_frame::tag() const
{
  return L4_msg_tag(i0);
}

IMPLEMENT inline
void Syscall_frame::tag(L4_msg_tag const &tag)
{
  i0 = tag.raw();
}

