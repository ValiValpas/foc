INTERFACE[sparc]:

#include "types.h"
#include "warn.h"

EXTENSION class Syscall_frame
{
  public:
    // FIXME adapt to sparc
//    Mword r[30]; //{r0, r2, r3, ..., r10, r13 .., r31, ip
    void dump() const;
};

EXTENSION class Return_frame
{
  // as in kern/sparc/crt0.S
  public:
    Mword reserved[8]; //+64 to +92
    Mword i7;   //+60
    Mword i6;   //+56
    Mword i5;   //+52
    Mword i4;   //+48
    Mword i3;   //+44
    Mword i2;   //+40
    Mword i1;   //+36
    Mword i0;   //+32
    Mword l7;   //+28
    Mword l6;   //+24
    Mword l5;   //+20
    Mword l4;   //+16
    Mword l3;   //+12
    Mword l2;   //+8
    Mword l1;   //+4
    Mword l0;   //0
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
  printf("L0 %08lx L1 %08lx L2 %08lx\n",
         l0, l1, l2);
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
  // FIXME imlement
  NOT_IMPL_PANIC;
  return Invalid_address;
//  return Return_frame::usp;
}

IMPLEMENT inline
void
Return_frame::sp(Mword _sp)
{
  // FIXME imlement
  (void)_sp;
  NOT_IMPL_PANIC;
//  Return_frame::usp = _sp;
}

IMPLEMENT inline
Mword
Return_frame::ip() const
{
  // FIXME imlement
  NOT_IMPL_PANIC;
  return Invalid_address;
//  return Return_frame::srr0;
}

IMPLEMENT inline
void
Return_frame::ip(Mword _pc)
{
  // FIXME imlement
  (void)_pc;
  NOT_IMPL_PANIC;
//  Return_frame::srr0 = _pc;
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
  NOT_IMPL_PANIC;
  (void)id;
//  r[5] = id; /*r6*/
}

IMPLEMENT inline
Mword Syscall_frame::from_spec() const
{
  NOT_IMPL_PANIC;
//  return r[5]; /*r6*/
}


IMPLEMENT inline
L4_obj_ref Syscall_frame::ref() const
{
  NOT_IMPL_PANIC;
//  return L4_obj_ref::from_raw(r[3]); /*r4*/
}

IMPLEMENT inline
void Syscall_frame::ref(L4_obj_ref const &ref)
{
  NOT_IMPL_PANIC;
  (void)ref;
//  r[3] = ref.raw(); /*r4*/
}

IMPLEMENT inline
L4_timeout_pair Syscall_frame::timeout() const
{ 
  NOT_IMPL_PANIC;
//  return L4_timeout_pair(r[4]); /*r5*/
}


IMPLEMENT inline 
void Syscall_frame::timeout(L4_timeout_pair const &to)
{
  NOT_IMPL_PANIC;
  (void)to;
//  r[4] = to.raw(); /*r5*/
}

IMPLEMENT inline Utcb *Syscall_frame::utcb() const
{
  NOT_IMPL_PANIC;
//  return reinterpret_cast<Utcb*>(r[1]); /*r2*/
}

IMPLEMENT inline L4_msg_tag Syscall_frame::tag() const
{
  NOT_IMPL_PANIC;
//  return L4_msg_tag(r[2]); /*r3*/
}

IMPLEMENT inline
void Syscall_frame::tag(L4_msg_tag const &tag)
{
  NOT_IMPL_PANIC;
  (void)tag;
//  r[2] = tag.raw(); /*r3*/
}

