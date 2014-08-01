INTERFACE [sparc]:

#include "entry_frame.h"

extern "C"
Mword
pagefault_entry(Address, Mword, Mword, Return_frame *);

EXTENSION class Mem_space
{
public:

  typedef Pdir Dir_type;

  /** Return status of v_insert. */
  enum //Status 
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  /** Attribute masks for page mappings. */
  enum Page_attrib
    {
      Page_no_attribs = 0,
      /// Page is writable.
      Page_writable = 0, // XXX
      Page_cacheable = 0,
      /// Page is noncacheable.
      Page_noncacheable = 0, // XXX
      /// it's a user page.
      Page_user_accessible = 0, // XXX
      /// Page has been referenced
      Page_referenced = Pte_base::Referenced,
      /// Page is dirty
      Page_dirty = Pte_base::Modified,
      Page_references = Pte_base::Referenced| Page_dirty,
      /// A mask which contains all mask bits
      Page_all_attribs = Page_writable | Page_noncacheable |
			 Page_user_accessible | Page_referenced | Page_dirty,
    };

  // Mapping utilities
  enum				// Definitions for map_util
  {
    Need_insert_tlb_flush = 0,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Map_superpage_size = Config::SUPERPAGE_SIZE,
    Map_max_address = Mem_layout::User_max,
    Whole_space = MWORD_BITS,
    Identity_map = 0,
  };

protected:
  // DATA
  Dir_type *_dir;
};


//----------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "mem_layout.h"
#include "paging.h"
#include "std_macros.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "logdefs.h"
#include "panic.h"
#include "lock_guard.h"
#include "cpu_lock.h"
#include "warn.h"




PUBLIC explicit inline
Mem_space::Mem_space(Ram_quota *q) : _quota(q), _dir(0) {}

PROTECTED inline NEEDS["kmem_alloc.h"]
bool
Mem_space::initialize()
{
  void *b;
  if (EXPECT_FALSE(!(b = Kmem_alloc::allocator()
	  ->q_alloc(_quota, Config::PAGE_SHIFT))))
    return false;

  _dir = static_cast<Dir_type*>(b);
  _dir->clear();	// initialize to zero
  return true; // success
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
: _quota(q), _dir(pdir)
{
  _kernel_space = this;
  _current.cpu(0) = this;
}

IMPLEMENT inline
void
Mem_space::make_current()
{
  extern Mword context_table[16];
  _current.cpu(current_cpu()) = this;

  // Currently, we simply alternate between context 0 and 1, invalidating
  // the old context entirely.

  // read current context number
  Mword cur_context = Mem_unit::context();
  Mword new_context = 1;
  if (cur_context != 0) {
    new_context = 0;
  }

  // update context table entry
  Pte_base root(&context_table[new_context], 0);
  root.set_next_level(virt_to_phys((Address)_dir)); // TODO pre-calc phys address (cp. arm)
  assert(virt_to_phys((Address)_dir) != Invalid_address);
  
  // invalidate old context
  Mem_unit::tlb_flush_context();

  // switch context
  Mem_unit::context(new_context);

  // no need to flush/invalidate the cache because it is context-aware

  /* We can do better by not flushing the context and 
   * applying an NRU replacement scheme.
   *
   * What we need to store:
   *  - A bitfield which stores the validity of every context number.
   *  - A bitfield which stores the used status of every context number.
   *  - The context number for (within) each context object.
   *
   * The algorithm:
   *  - Check if the context number is still valid and points to the right pdir.
   *    - YES: Switch to this number, mark as used.
   *    - NO:  Still an invalid context number available?
   *        - YES: Store entry in context table, switch context number, mark as used.
   *        - NO:  Unused context numbers available?
   *            - NO:  Clear "used" bitfield. Take any context number but the current.
   *            Invalidate the selected context.
   *            Store new entry in context table, switch context number, mark as used.
   *
   * TODO implement NRU replacement for sparc contexts
   */
}


PROTECTED inline
void
Mem_space::sync_kernel()
{
  _dir->sync(Virt_addr(Mem_layout::User_max), kernel_space()->_dir,
             Virt_addr(Mem_layout::User_max),
             Virt_addr(-Mem_layout::User_max), Pdir::Super_level,
             Kmem_alloc::q_allocator(_quota));
}


IMPLEMENT inline NEEDS ["kmem.h"]
void Mem_space::switchin_context(Mem_space *from)
{
  if (from != this)
    make_current();
}

//XXX cbass: check;
PUBLIC static inline
Mword
Mem_space::xlate_flush(unsigned char rights)
{
  Mword a = Page_references;
  if (rights & L4_fpage::RX)
    a |= Page_all_attribs;
  else if (rights & L4_fpage::W)
    a |= Page_writable;

  return a;
}

PUBLIC static inline
Mword
Mem_space::is_full_flush(unsigned char rights)
{
  return rights & L4_fpage::RX;
}

PUBLIC static inline
unsigned char
Mem_space::xlate_flush_result(Mword attribs)
{
  unsigned char r = 0;
  if (attribs & Page_referenced)
    r |= L4_fpage::RX;

  if (attribs & Page_dirty)
    r |= L4_fpage::W;

  return r;
}

PUBLIC inline NEEDS["cpu.h"]
static bool
Mem_space::has_superpages()
{
  return Cpu::have_superpages();
}

//we flush tlb in htab implementation
PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool = false)
{
  Mem_unit::tlb_flush();
}



PUBLIC inline
bool 
Mem_space::set_attributes(Virt_addr virt, unsigned page_attribs)
{
  auto i = _dir->walk(virt);

  if (!i.e->is_valid())
    return false;

  i.e->set_attribs(page_attribs);
  // TODO flush tlb?
  return true;
}

PROTECTED inline
void
Mem_space::destroy()
{}

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PRIVATE
void
Mem_space::dir_shutdown()
{

  // free ldt memory if it was allocated
  //free_ldt_memory();

  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared
  /*
  _dir->alloc_cast<Mem_space_q_alloc>()
    ->destroy(0, Kmem::mem_user_max, Pdir::Depth - 1,
              Mem_space_q_alloc(_quota, Kmem_alloc::allocator()));
*/
  NOT_IMPL_PANIC;
}

IMPLEMENT inline
Mem_space *
Mem_space::current_mem_space(unsigned cpu) /// XXX: do not fix, deprecated, remove!
{
  return _current.cpu(cpu);
}

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB.
    @param page_attribs Attributes for the mapping (see
                        Mem_space::Page_attrib).
    @return Insert_ok if a new mapping was created;
             Insert_warn_exists if the mapping already exists;
             Insert_warn_attrib_upgrade if the mapping already existed but
                                        attributes could be upgraded;
             Insert_err_nomem if the mapping could not be inserted because
                              the kernel is out of memory;
             Insert_err_exists if the mapping could not be inserted because
                               another mapping occupies the virtual-address
                               range
    @pre phys and virt need to be size-aligned according to the size argument.
 */
IMPLEMENT inline
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Vsize size,
		    unsigned page_attribs, bool /*upgrade_ignore_size*/)
{
//  bool const flush = _current.current() == this;
  assert (phys.offset(size) == 0);
  assert (virt.offset(size) == 0);

  int level;
  for (level = 0; level <= Pdir::Depth; ++level) {
    Vsize page_size(1 << Pte_base::page_order_for_level(level));
    if (page_size <= size)
      break;
  }

  auto i = _dir->walk(virt, level, Kmem_alloc::q_allocator(_quota));

  if (EXPECT_FALSE(!i.e->is_valid() && i.e->level != level))
    return Insert_err_nomem;

  if (EXPECT_FALSE(i.e->is_valid()
                   && (i.e->level != level || Phys_addr(i.e->page_addr()) != phys)))
    return Insert_err_exists;

  if (i.e->is_valid())
    {
      if (EXPECT_FALSE(!i.e->add_attribs(page_attribs)))
        return Insert_warn_exists;

      return Insert_warn_attrib_upgrade;
    }
  else
    {
      i.e->create_page(phys, page_attribs);

      return Insert_ok;
    }
}


/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Mem_space::virt_to_phys (Address virt) const
{
  return dir()->virt_to_phys(virt);
}

PUBLIC inline
virtual Address
Mem_space::virt_to_phys_s0(void *a) const
{
  return dir()->virt_to_phys((Address)a);
}

PUBLIC inline
Address
Mem_space::pmem_to_phys (Address virt) const
{
  return virt_to_phys(virt);
}



/** Look up a page-table entry.
    @param virt Virtual address for which we try the look up.
    @param phys Meaningful only if we find something (and return true).
                If not 0, we fill in the physical address of the found page
                frame.
    @param page_attribs Meaningful only if we find something (and return true).
                If not 0, we fill in the page attributes for the found page
                frame (see Mem_space::Page_attrib).
    @param size If not 0, we fill in the size of the page-table slot.  If an
                entry was found (and we return true), this is the size
                of the page frame.  If no entry was found (and we
                return false), this is the size of the free slot.  In
                either case, it is either 4KB or 4MB.
    @return True if an entry was found, false otherwise.
 */
IMPLEMENT
bool
Mem_space::v_lookup(Vaddr virt, Phys_addr *phys = 0, Size *size = 0,
		    unsigned *page_attribs = 0)
{
  auto i = _dir->walk(virt);

  if (!i.e->is_valid())
    return false;

  if (phys) *phys = Phys_addr(i.e->page_addr());
  if (page_attribs) *page_attribs = i.e->attribs();
  if (size) *size = Size(1 << i.e->page_order());

  return true;
}

/** Delete page-table entries, or some of the entries' attributes.  This
    function works for one or multiple mappings (in contrast to v_insert!).
    @param virt Virtual address of the memory region that should be changed.
    @param size Size of the memory region that should be changed.
    @param page_attribs If nonzero, delete only the given page attributes.
                        Otherwise, delete the whole entries.
    @return Combined (bit-ORed) page attributes that were removed.  In
            case of errors, ~Page_all_attribs is additionally bit-ORed in.
 */
IMPLEMENT
unsigned long
Mem_space::v_delete(Vaddr virt, Vsize size,
		    unsigned long page_attribs = Page_all_attribs)
{
  assert (virt.offset(size) == 0);
  auto i = _dir->walk(virt);

  if (EXPECT_FALSE (! i.e->is_valid()))
    return L4_fpage::Rights(0);

  Page::Attribs ret = i.e->access_flags();

  if (page_attribs)
    i.e->del_rights(page_attribs);
  else
    i.e->clear();

  // FIXME return value
  return ret;
}

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }
