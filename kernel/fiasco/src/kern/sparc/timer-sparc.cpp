/**
 * Generic sparc timer
 */

INTERFACE [sparc]:

EXTENSION class Timer
{
private:
    static inline void update_one_shot(Unsigned64 wakeup);
};

IMPLEMENTATION [sparc]:

#include "cpu.h"
#include "config.h"
#include "globals.h"
#include "kip.h"
#include "watchdog.h"

IMPLEMENT inline NEEDS ["kip.h"]
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline NEEDS ["globals.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    return 0;
  else
    return Kip::k()->clock;
}

IMPLEMENT inline NEEDS ["config.h", "kip.h", "watchdog.h"]
void
Timer::update_system_clock(unsigned cpu)
{
  if (cpu == 0)
    {
      Kip::k()->clock += Config::Scheduler_granularity;
      // FIXME do we need to invalidate the data cache?
      Watchdog::touch();
    }
}

IMPLEMENT inline NEEDS[Timer::update_one_shot, "config.h"]
void
Timer::update_timer(Unsigned64 wakeup)
{
  if (Config::Scheduler_one_shot)
    update_one_shot(wakeup);
}
