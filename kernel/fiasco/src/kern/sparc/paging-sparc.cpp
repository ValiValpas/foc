INTERFACE[sparc]:

#include "types.h"
#include "config.h"

class PF {};

class Fsr {
public:
  /// MMU fault status register bits, see sparc v8 manual p.256ff
  enum Fsr_bits {
    Ext_bus       = 10,
    Level         =  8,
    Access_type   =  5,
    Fault_type    =  2,
    Address_valid =  1,
    Overwrite     =  0
  };

  /// MMU fault status register bit masks, see sparc v8 manual p.256ff
  enum Fsr_masks {
    Ext_bus_mask          = 0xFF << 10,
    Level_mask            = 0x3  << 8,
    Access_type_mask      = 0x7  << 5,
    Fault_type_mask       = 0x7  << 2,
    Supervisor_mask       = 0x20,
    Access_store_mask     = 0x80,
    Reserved_mask         = 0x3FFF << 18
  };

  /// MMU fault types, see sparc v8 manual p.256ff
  enum Fault_types {
    None            = 0x0,
    Invalid_addr    = 0x1,
    Protection      = 0x2,
    Privilege_viol  = 0x3,
    Translation     = 0x4,
    Access_bus      = 0x5,
    Internal        = 0x6,
    Reserved        = 0x7
  };

  /// MMU access types, see sparc v8 manual p.256ff
  enum Access_types {
    User_data_load   = 0x0,
    Super_data_load  = 0x1,
    User_inst_load   = 0x2,
    Super_inst_load  = 0x3,
    User_data_store  = 0x4,
    Super_data_store = 0x5,
    User_inst_store  = 0x6,
    Super_inst_store = 0x7
  };

  /// reserved bits
  enum Reserved_bits {
    User_mode = 31
  };
};

//------------------------------------------------------------------------------
INTERFACE[sparc]:

#include <cassert>
#include "types.h"
#include "ptab_base.h"
#include "kdb_ke.h"
#include "mem_unit.h"

//#define THIS_NEEDS_ADAPTION
//#define THIS_NEED_ADAPTION

//EXTENSION class Page                                                                                                                                             
//{                                                                                                                                                                
//public:                                                                                                                                                          
//  enum Attribs_enum                                                                                                                                              
//  {                                                                                                                                                              
//    MAX_ATTRIBS   = 0x00000006,                                                                                                                                  
//    Cache_mask    = 0x00000018, ///< Cache attrbute mask                                                                                                         
//    CACHEABLE     = 0x00000080,                                                                                                                                  
//    BUFFERED      = 0x00000010,                                                                                                                                  
//    NONCACHEABLE  = 0x00000018,                                                                                                                                  
//  };                                                                                                                                                             
//};

class Paging {};

/**
 * @remark: SRMMU uses 36-bit physical addresses
 */
class Pte_base
{
public:
  typedef Mword Entry;

  Pte_base() = default;
  Pte_base(void *p, unsigned char level) : pte((Mword*)p), level(level) {}

  bool is_valid() const { return *pte & Valid; }

  void clear() { *pte = 0; }

  bool is_leaf() const
  {
    return *pte & ET_pte;
  }

  Dword next_level() const
  {
    assert(*pte & ET_ptd);
    return (*pte & Ptp_mask) << Ptp_addr_shift;
  };

  /**
   * Set next level page table
   */
  void set_next_level(Dword phys)
  {
    Mword shifted = phys >> Ptp_addr_shift;
    assert((shifted & ~Ptp_mask) == 0);
    write_now(pte, shifted | ET_ptd);
  }

  /**
   * Set next level page table
   */
  void set_next_level(Mword *ptr)
  {
    Dword phys = (Mword)ptr;
    set_next_level(phys);
  }

  unsigned char page_order() const
  {
    switch (level)
    {
      // context table would be level -1
      case 0:
        return 24;
      case 1:
        return 18;
      default:     // level >= 2
        return 12;
    }
  }

  /**
   * Get physical page address (36-bit)
   */
  Dword page_addr() const
  {
    assert(*pte & ET_pte);
    Dword paddr = (*pte & Ppn_mask) << Ppn_addr_shift;
    return cxx::mask_lsb(paddr, page_order());
  }

  enum
  {
    ET_ptd         = 1,           ///< PT Descriptor is PTD
    ET_pte         = 2,           ///< PT Descriptor is PTE

    Ptp_mask       = 0xfffffffc,  ///< PTD: page table pointer
    Ppn_mask       = 0xffffff00,  ///< PTE: physical page number
    Ptp_addr_shift = 4,           ///< PTD: need to shift phys addr
    Ppn_addr_shift = 4,           ///< PTE: need to shift phys addr
    Cacheable      = 0x80,        ///< PTE: is cacheable
    Modified       = 0x40,        ///< PTE: modified
    Referenced     = 0x20,        ///< PTE: referenced
    Accperm_mask   = 0x1C,        ///< 3 bits for access permissions
    Accperm_shift  = 2,
    Accperm_RO     = 0x0,         ///< read only (user/supervisor)
    Accperm_RW     = 0x1,         ///< read write (user/supervisor)
    Accperm_RX     = 0x2,         ///< read execute (user/supervisor)
    Accperm_RWX    = 0x3,         ///< read write execute (user/supervisor)
    Accperm_XO     = 0x4,         ///< execute only (user/supervisor)
    Accperm_RO_RW  = 0x5,         ///< read only (user), read write (supervisor)
    Accperm_NO_RX  = 0x6,         ///< no access (user), read execute (supervisor)
    Accperm_NO_RWX = 0x7,         ///< no access (user), read write execute (supervisor)
    Et_mask        = 0x3,         ///< 2 bits to determine entry type
    Vfpa_mask      = 0xfffff000,  ///< Flush/Probe: virt. addr. mask
    Flushtype_mask = 0xf00,       ///< Flush/Probe: type

//    Pdir_mask        = 0xFF,
//    Pdir_shift       = 24,
//    Ptab_mask        = 0x3F,
//    Ptab_shift1      = 18,
//    Ptab_shift2      = 12,
//    Page_offset_mask = 0xFFF,

    Super_level    = 0,
    Valid          = 0x3,        ///< ET field mask
  };

namespace Page
{
  typedef Unsigned32 Attribs;
  enum Attribs_enum
  {
    KERN_RW      = 0x00000000,
    USER_RO      = 0x00000001,
    USER_RW      = 0x00000002,
    Cache_mask   = 0x00000078,
    CACHEABLE    = 0x00000000,
    NONCACHEABLE = 0x00000040,
    BUFFERED     = 0x00000080, //XXX not sure
  };
};

  Mword *pte;
  unsigned char level;

};

typedef Ptab::Tupel< Ptab::Traits<Unsigned32, 24, 8, true>,
                     Ptab::Traits<Unsigned32, 18, 6, true>,
                     Ptab::Traits<Unsigned32, 12, 6, true> >::List Ptab_traits;

typedef Ptab::Shift<Ptab_traits, Virt_addr::Shift>::List Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Page_number, Virt_addr::Shift> Ptab_va_vpn;


IMPLEMENTATION[sparc]:

#include "psr.h"
#include "lock_guard.h"
#include "cpu_lock.h"
#include "kip.h"

PRIVATE inline
Mword
Pte_base::_attribs(Page::Attr attr) const
{
  static const unsigned short perms[] = {
      Accperm_NO_RX  << Accperm_shift, // 0000: none, hmmm
      Accperm_NO_RX  << Accperm_shift, // 000X: kernel rx
      Accperm_NO_RWX << Accperm_shift, // 00W0: kernel rwx (no rw)
      Accperm_NO_RWX << Accperm_shift, // 00WX: kernel rwx

      Accperm_NO_RX  << Accperm_shift, // 0R00: kernel rx (no ro)
      Accperm_NO_RX  << Accperm_shift, // 0R0X:
      Accperm_NO_RWX << Accperm_shift, // 0RW0: kernel rwx (no rw)
      Accperm_NO_RWX << Accperm_shift, // 0RWX:

      Accperm_NO_RX  << Accperm_shift, // U000:
      Accperm_XO     << Accperm_shift, // U00X: 
      Accperm_RW     << Accperm_shift, // U0W0:
      Accperm_RWX    << Accperm_shift, // U0WX:

      Accperm_RO     << Accperm_shift, // UR00:
      Accperm_RX     << Accperm_shift, // UR0X: 
      Accperm_RW     << Accperm_shift, // URW0:
      Accperm_RWX    << Accperm_shift  // URWX:
  };

  typedef Page::Type T;
  Mword r = 0;
  if (attr.type == T::Normal())   r |= Cacheable;
  if (attr.type == T::Uncached()) r &= ~Cacheable;

  return r | perms[cxx::int_value<L4_fpage::Rights>(attr.rights)];
}

PUBLIC inline
Page::Attr
Pte_base::attribs() const
{
  auto r = access_once(pte);
  auto c = r & Cacheable;
  r = (r & Accperm_mask) >> Accperm_shift;

  typedef L4_fpage::Rights R;
  typedef Page::Type T;

  R rights;
  switch (r)
    {
    case Accperm_RO:     rights = R::UR();           break;
    case Accperm_RX:     rights = R::URX();          break;
    case Accperm_RW:     rights = R::URW();          break;
    case Accperm_RWX:    rights = R::URWX();         break;
    case Accperm_NO_RX:  rights = R::RX();           break;
    case Accperm_NO_RWX: rights = R::RWX();          break;
    case Accperm_RO_RW:  rights = R::UR();           break;
    case Accperm_XO:     rights = R::U() | R::X();   break;
    }

  T type;
  switch (c)
    {
    case Cacheable: type = T::Normal();   break;
    default:        type = T::Uncached(); break;
    }

  return Page::Attr(rights, type);
}

PUBLIC inline NEEDS[Pte_base::_attribs]
void
Pte_base::set_attribs(Page::Attr attr)
{
  Mword p    = access_once(pte);
  Mword mask = ~(Accperm_mask | Cacheable);
  p = (p & mask) | _attribs(attr);
  write_now(pte, p);
}

PUBLIC inline NEEDS[Pte_base::_attribs]
void
Pte_base::create_page(Phys_mem_addr addr, Page::Attr attr)
{
  Dword paddr = cxx::int_value<Phys_mem_addr>(addr);
  assert(cxx::get_lsb(paddr, page_order()) == 0);
  Mword p = ET_pte | _attribs(attr) | (Mword)(paddr >> Ptp_addr_shift);
  write_now(pte, p);
}

PUBLIC inline NEEDS[Pte_base::_attribs]
bool
Pte_base::add_attribs(Page::Attr attr)
{
  typedef L4_fpage::Rights R;

  auto p = access_once(pte);
  auto o = (p & Accperm_mask) >> Accperm_shift;
  R r(attr.rights);

  // make sure we don't remove rights
  switch (o)
  {
    case Accperm_RO:
      if (r == R::RW())
        o = Accperm_RO_RW;
      else if ((r & R::W()) && (r & R::X()))
        o = Accperm_RWX;
      else if (r & R::W())
        o = Accperm_RW;
      else if (r & R::X())
        o = Accperm_RX;
      else
        return false;
      break;
    case Accperm_RW:
      if (r & R::X())
        o = Accperm_RWX;
      else
        return false;
      break;
    case Accperm_RX:
      if (r & R::W())
        o = Accperm_RWX;
      else
        return false;
      break;
    case Accperm_RWX:
      return false;
    case Accperm_XO:
      if (r & R::W())
        o = Accperm_RWX;
      else if (r & R::R())
        o = Accperm_RX;
      else
        return false;
      break;
    case Accperm_RO_RW:
      if (r & R::X())
        o = Accperm_RWX;
      else if ((r & R::U()) && (r & R::W()))
        o = Accperm_RW;
      else
        return false;
      break;
    case Accperm_NO_RX:
      if (r & R::U()) {
        if (r & R::W())
          o = Accperm_RWX;
        else if (r & R::X())
          o = Accperm_RX;
        else
          return false;
      }
      else if (r & R::W())
        o = Accperm_NO_RWX;
      else
        return false;
      break;
    case Accperm_NO_RWX:
      if (r & R::U()) {
        if (r & R::RWX())
          o = Accperm_RWX;
        else
          return false;
      }
      else
        return false;
      break;
  }

  p = (p & ~Accperm_mask) | (o << Accperm_shift);
  write_now(pte, p);
  return true;
}

PUBLIC inline
Page::Rights
Pte_base::access_flags() const
{ return Page::Rights(0); } // FIXME sparc: Pte_ptr::access_flags()

PUBLIC inline
void
Pte_base::del_rights(L4_fpage::Rights r)
{
  typedef L4_fpage::Rights R;

  auto p = access_once(pte);
  auto o = (p & Accperm_mask) >> Accperm_shift;

  switch (o)
  {
    case Accperm_RO: // can't remove anything
      return;
    case Accperm_RX:
      if (r & R::R())
        o = Accperm_XO;
      else if (r & R::X())
        o = Accperm_RO;
      else
        return;
      break;
    case Accperm_RW:
      if (r & R::W())
        o = Accperm_RO;
      else
        return;
      break;
    case Accperm_RWX:
      if (r & R::W()) {
        if (r & R::X())
          o = Accperm_RO;
        else if (r & R::R())
          o = Accperm_XO;
        else
          o = Accperm_RX;
      }
      else if (r & R::X())
        o = Accperm_RW;
      else
        return;
      break;
    case Accperm_XO: // can't remove anything
      return;
    case Accperm_RO_RW:
      if (r & R::W())
        o = Accperm_RO;
      else
        return;
      break;
    case Accperm_NO_RX: // can't remove anything
      return;
    case Accperm_NO_RWX:
      if (r & R::W())
        o = Accperm_NO_RX;
      else
        return;
      break;
  }

  p = (p & ~Accperm_mask) | o;
  write_now(pte, p);
}

/* this functions do nothing on SPARC architecture */
PUBLIC static inline
Address
Paging::canonize(Address addr)
{
  return addr;
}

PUBLIC static inline
Address
Paging::decanonize(Address addr)
{
  return addr;
}

//---------------------------------------------------------------------------
IMPLEMENT inline
Mword PF::is_translation_error(Mword error)
{
  Mword fault_type = (error & Fsr::Fault_type_mask) >> Fsr::Fault_type;
  return fault_type == Fsr::Invalid_addr;
}

IMPLEMENT inline
Mword PF::is_usermode_error(Mword error)
{
  return (error & (1 << Fsr::User_mode)) != 0;
}

IMPLEMENT inline
Mword PF::is_read_error(Mword error)
{
  bool is_store = error & Fsr::Access_store_mask;
  return !is_store;
}

IMPLEMENT inline
Mword PF::addr_to_msgword0(Address pfa, Mword error)
{
  Mword a = pfa & ~3;
  if(is_translation_error(error))
    a |= 1;
  if(!is_read_error(error))
    a |= 2;
  return a;
}

//---------------------------------------------------------------------------

PUBLIC
Pte_base::Pte_base(Mword raw) : _raw(raw) {}

PUBLIC
Pte_base::Pte_base() {}

PUBLIC inline
Pte_base &
Pte_base::operator = (Pte_base const &other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pte_base &
Pte_base::operator = (Mword raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Mword
Pte_base::raw() const
{
  return _raw;
}

PUBLIC inline
void
Pte_base::add_attr(Mword attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pte_base::del_attr(Mword attr)
{
  _raw &= ~attr;
}

PUBLIC inline
void
Pte_base::clear()
{ _raw = 0; }

PUBLIC inline
int
Pte_base::valid() const
{
  return _raw & Valid;
}

PUBLIC inline
int
Pte_base::writable() const
{
  Mword acc = ((_raw >> Accperm_shift) & Accperm_mask);
  return (acc=1); // XXX (3 and 5 can also be valid, depending on access mode)
}

PUBLIC inline
Address
Pt_entry::pfn() const
{
  return _raw & Ppn_mask;
}

PUBLIC static inline
Mword
Pte_base::pdir(Address a)
{
  return (a & (Pdir_mask << Pdir_shift)) >> Pdir_shift;
}


PUBLIC static inline
Mword
Pte_base::ptab1(Address a)
{
  return (a & (Ptab_mask << Ptab_shift1)) >> Ptab_shift1;
}


PUBLIC static inline
Mword
Pte_base::ptab2(Address a)
{
  return (a & (Ptab_mask << Ptab_shift2)) >> Ptab_shift2;
}


PUBLIC static inline
Mword
Pte_base::offset(Address a)
{
  return a & Page_offset_mask;
}

PUBLIC static inline
void
Paging::split_address(Address a)
{
  printf("%lx -> %lx : %lx : %lx : %lx\n", a,
         Pte_base::pdir(a),  Pte_base::ptab1(a),
         Pte_base::ptab2(a), Pte_base::offset(a));
}

Mword context_table[16];
// FIXME move kernel_srmmu_l1 somewhere sensible (or alloc from kmem_alloc?)
Mword kernel_srmmu_l1[256] __attribute__((aligned(0x400)));

