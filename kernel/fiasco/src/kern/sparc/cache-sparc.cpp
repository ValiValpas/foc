INTERFACE:

#include "types.h"

class Cache
{
public:
  static void init();

  static void flush_dcache();
  static void flush_icache();
  static void flush_caches();
};
