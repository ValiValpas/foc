INTERFACE [sparc]:

#include "types.h"
#include "sparc_types.h"

EXTENSION class Boot_info 
{
  public:
    /**
     * Return memory-mapped base address of uart/pic
     */
    static Address pic_base();
    static Address timer_base();

  private:
    static Address _pic_base;
    static Address _timer_base;
    static void lookup_devices();
};


//------------------------------------------------------------------------------
IMPLEMENTATION [sparc && !debug]:

#include "boot_info.h"
#include <string.h>

IMPLEMENT static 
void Boot_info::init()
{
  lookup_devices();
}

IMPLEMENTATION [sparc && debug]:

#include "boot_info.h"
#include <string.h>

IMPLEMENT static 
void Boot_info::init()
{
  lookup_devices();
  Boot_info::dump();
}
