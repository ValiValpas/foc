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


class Paging {};


namespace Page
{
  typedef Unsigned32 Attribs;
  enum Attribs_enum
  {
    USER_RO  = 0x00000000, ///< User Read only
    USER_RW  = 0x00000001, ///< User Read/Write
    USER_RX  = 0x00000002, ///< User Read/Execute
    USER_RWX = 0x00000003, ///< User Read/Write/Execute
    USER_XO  = 0x00000004, ///< User Execute only
	 KERN_RX  = 0x00000006,
	 KERN_RWX = 0x00000007,
    USER_NO  = 0x00000007, ///< User No access

    NONCACHEABLE = 0x00000000, ///< Caching is off
    CACHEABLE    = 0x00000080, ///< Cache is enabled
    BUFFERED     = 0x00000080, ///< Cache is enabled
    Cache_mask   = 0x00000080, ///< Cacheable bit
  };
};

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
  bool valid() const { return is_valid(); }

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

  Address addr() const {
	  if (is_leaf())
		  return page_addr();
	  else
		  return next_level();
  }

  unsigned char page_order() const
  {
	  return page_order_for_level(level);
  }

  static
  unsigned char page_order_for_level(unsigned lvl)
  {
    switch (lvl)
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

	static inline
	bool
	need_cache_write_back(bool current_pt)
	{ return true; /*current_pt;*/ (void)current_pt; }

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

  Mword *pte;
  unsigned char level;

};

template<unsigned SHIFT, unsigned LVL>
class Pt_entry : public Pte_base
{
public:
  typedef Mword Raw;
  enum { Page_shift = SHIFT };
  Mword leaf() const { return is_leaf(); }
  Mword raw() const { return access_once(pte); }
  Pt_entry() { Pte_base::level = LVL; }
  void set(Address p, bool intermed, bool, unsigned long attrs = 0)
  {
	  if (intermed)
		  set_next_level(p);
	  else
		  create_page(Phys_addr(p), attrs);
  }
};

typedef Ptab::Tupel< Ptab::Traits<Pt_entry<24, 0>, 24, 8, true>,
                     Ptab::Traits<Pt_entry<24, 1>, 18, 6, true>,
                     Ptab::Traits<Pt_entry<24, 2>, 12, 6, true> >::List Ptab_traits;

typedef Ptab::Shift<Ptab_traits, Virt_addr::Shift>::List Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Page_number, Virt_addr::Shift> Ptab_va_vpn;


IMPLEMENTATION[sparc]:

#include "psr.h"
#include "lock_guard.h"
#include "cpu_lock.h"
#include "kip.h"



PUBLIC inline
void
Pte_base::set_attribs(Page::Attribs attr)
{
  Mword p    = access_once(pte);
  Mword mask = ~(Accperm_mask | Cacheable);
  p = (p & mask) | attr;
  write_now(pte, p);
}

PUBLIC inline
void
Pte_base::create_page(Phys_addr addr, Page::Attribs attr)
{
  assert(addr.offset(Phys_addr(1 << page_order())) == 0);
  Mword p = ET_pte | attr | (Mword)(addr >> Ptp_addr_shift).value();
  write_now(pte, p);
}

PUBLIC inline
bool
Pte_base::add_attribs(Page::Attribs attr)
{

  auto p = access_once(pte);
  auto o = (p & Accperm_mask) >> Accperm_shift;
  auto r = attr & 0x7;

  if (o != r) {
	  // make sure we don't remove rights
	  switch (o)
	  {
		 case Accperm_RO: /* USER_RO */
			if (r == Page::USER_RW || r == Page::USER_RWX || r == Page::USER_RX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_RW: /* USER_RW */
			if (r == Page::USER_RWX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_RX: /* USER_RX */
			if (r == Page::USER_RWX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_RWX:
			return false;
		 case Accperm_XO: /* USER_XO */
			if (r == Page::USER_RWX || r == Page::USER_RX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_RO_RW:
			if (r == Page::KERN_RWX || r == Page::USER_RW || r == Page::USER_RWX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_NO_RX: /* KERN_RX */
			if (r == Page::KERN_RWX || r == Page::USER_RX || r == Page::USER_RWX)
				o = r;
			else
			  return false;
			break;
		 case Accperm_NO_RWX: /* KERN_RWX */
			if (r == Page::USER_RWX)
				o = r;
			else
			  return false;
			break;
	  }

	  p = (p & ~Accperm_mask) | (o << Accperm_shift);
	  write_now(pte, p);
  }
  return true;
}

PUBLIC inline
Page::Attribs
Pte_base::access_flags() const
{ return (access_once(pte) & Accperm_mask) >> Accperm_shift; }

PUBLIC inline
Page::Attribs
Pte_base::attribs() const
{ return access_flags() | (access_once(pte) & Cacheable); }

PUBLIC inline
void
Pte_base::del_rights(Page::Attribs attr)
{
  auto p = access_once(pte);
  auto o = (p & Accperm_mask) >> Accperm_shift;
  auto r = attr & 0x7;

  switch (o)
  {
    case Accperm_RO: // can't remove anything
      return;
    case Accperm_RX: /* USER_RX */
		if (r == Page::USER_RO || r == Page::USER_XO)
			o = r;
      else
        return;
      break;
    case Accperm_RW: /* USER_RW */
		if (r == Page::USER_RO)
			o = r;
      else
        return;
      break;
    case Accperm_RWX: /* USER_RWX */
		if (r == Page::USER_RX || r == Page::USER_RO || r == Page::USER_RW || r == Page::KERN_RWX || r == Page::KERN_RX || r == Page::USER_XO)
			o = r;
      else
        return;
      break;
    case Accperm_XO:
		if (r == Page::KERN_RX)
			o = r;
      return;
    case Accperm_RO_RW:
		if (r == Page::USER_RO)
			o = r;
      else
        return;
      break;
    case Accperm_NO_RX: // can't remove anything
      return;
    case Accperm_NO_RWX:
		if (r == Page::KERN_RX)
			o = r;
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

//PUBLIC inline
//void
//Pte_base::clear()
//{ _raw = 0; }
//
//PUBLIC inline
//int
//Pte_base::valid() const
//{
//  return _raw & Valid;
//}
//
//PUBLIC inline
//int
//Pte_base::writable() const
//{
//  Mword acc = ((_raw >> Accperm_shift) & Accperm_mask);
//  return (acc=1); // XXX (3 and 5 can also be valid, depending on access mode)
//}
//
//PUBLIC static inline
//Mword
//Pte_base::pdir(Address a)
//{
//  return (a & (Pdir_mask << Pdir_shift)) >> Pdir_shift;
//}
//
//
//PUBLIC static inline
//Mword
//Pte_base::ptab1(Address a)
//{
//  return (a & (Ptab_mask << Ptab_shift1)) >> Ptab_shift1;
//}
//
//
//PUBLIC static inline
//Mword
//Pte_base::ptab2(Address a)
//{
//  return (a & (Ptab_mask << Ptab_shift2)) >> Ptab_shift2;
//}
//
//
//PUBLIC static inline
//Mword
//Pte_base::offset(Address a)
//{
//  return a & Page_offset_mask;
//}
//
//PUBLIC static inline
//void
//Paging::split_address(Address a)
//{
//  printf("%lx -> %lx : %lx : %lx : %lx\n", a,
//         Pte_base::pdir(a),  Pte_base::ptab1(a),
//         Pte_base::ptab2(a), Pte_base::offset(a));
//}

Mword context_table[16];
// FIXME move kernel_srmmu_l1 somewhere sensible (or alloc from kmem_alloc?)
Mword kernel_srmmu_l1[256] __attribute__((aligned(0x400)));

