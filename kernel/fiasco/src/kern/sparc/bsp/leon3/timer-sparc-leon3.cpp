INTERFACE [sparc && leon3]:

#include "types.h"
#include "io.h"
#include "boot_info.h"
#include <cassert>

#define SPARC_CLOCK_RATE         40000000  /* 40 MHz */
#define SPARC_TIMER_PRESCALE_HZ   1000000  /*  1 MHz */

EXTENSION class Timer
{
private:
  ///+0x00: Scaler value
  static Address scaler_value()    { return _timer_base + 0x00; }
  ///+0x04: Scaler reload value
  static Address scaler_reload()   { return _timer_base + 0x04; }
  ///+0x08: Config register
  static Address config()          { return _timer_base + 0x08; }
  ///+0xn0: Timer n counter value
  static Address timer_value(unsigned n)
  {
    assert(n > 0 && n <= Timer_max);
    return _timer_base + n*0x10;
  }
  ///+0xn4: Timer n reload value
  static Address timer_reload(unsigned n)
  {
    assert(n > 0 && n <= Timer_max);
    return _timer_base + n*0x10 + 0x04;
  }
  ///+0xn8: Timer n control register
  static Address timer_control(unsigned n)
  {
    assert(n > 0 && n <= Timer_max);
    return _timer_base + n*0x10 + 0x08;
  }

  /** Config register bitmasks */
  enum Config_bits
  {
    Timers_mask       = 0x7,
    Irq_mask          = 0x1F,
    Irq_shift         = 3,
    Separate_irq_shift= 8,
    Disable_freeze    = 9,
  };

  /** Control register bits */
  enum Control_bits
  {
    Enable_bit      = 0,
    Restart_bit     = 1,
    Load_bit        = 2,
    Irq_enable_bit  = 3,
    Irq_pending_bit = 4,
    Chain_bit       = 5,
    Debug_halt_bit  = 6,
  };

  enum { Scaler_mask = 0xFFFF };

  static Address    _timer_base;
  static Mword      _irq_start;
  static Mword      _num_timers;

public:
  enum { Timer_max = 7 };

  static void dump();
//  static Irq_chip_icu *main;
};

//------------------------------------------------------------------------------
IMPLEMENTATION [sparc && leon3]:
#include "warn.h"

Address Timer::_timer_base = 0;
Mword   Timer::_irq_start  = 0;
Mword   Timer::_num_timers = 0;

IMPLEMENT inline NEEDS ["boot_info.h", "io.h", "kip.h", "config.h", <cstdio>]
void
Timer::init(Cpu_number)
{
  _timer_base = Boot_info::timer_base();

  Unsigned32 creg = Proc::read_alternative<Mmu::Bypass>(config());

  // init members
  _irq_start      = (creg >> Irq_shift) & Irq_mask;
  _num_timers     = creg & Timers_mask;

  assert(_num_timers > 0);

  // configure prescaler
  Unsigned32 prescale_reload = Config::System_clock / Config::Timer_prescale_hz;
  assert(prescale_reload <= Scaler_mask);
  Proc::write_alternative<Mmu::Bypass>(scaler_reload(), prescale_reload);
  
  // configure and enable timer 1 for scheduling tick
  Unsigned32 tmr1_reload  = Config::Timer_prescale_hz / Config::Scheduler_granularity;
  Proc::write_alternative<Mmu::Bypass>(timer_reload(1), tmr1_reload);
  
  Unsigned32 tmr1_config = (1 << Enable_bit) | (1 << Restart_bit) | (1 << Irq_enable_bit) | (1 << Load_bit) | (1 << Irq_pending_bit);
  Proc::write_alternative<Mmu::Bypass>(timer_control(1), tmr1_config);

//  // dump timer configuration
//  dump();
}

PUBLIC static inline
unsigned
Timer::irq()
{
  // assert(_irq_start > 0);

  // TODO support separate timer IRQs
  return _irq_start;
}

PUBLIC static inline
unsigned
Timer::irq_mode()
{ return 0; }

PUBLIC static inline
void
Timer::acknowledge()
{
  Unsigned32 tmr1_config = Proc::read_alternative<Mmu::Bypass>(timer_control(1));

  tmr1_config |= (1 << Irq_pending_bit);
  Proc::write_alternative<Mmu::Bypass>(timer_control(1), tmr1_config);
}

IMPLEMENT inline NEEDS ["warn.h"]
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  // FIXME implement one shot timer for LEON3
  //   could be done by chaining timer 1+2 in order to have a 64bit timer
  (void)wakeup;
  NOT_IMPL_PANIC;
//  Unsigned64 now = Kip::k()->clock;
//  Mword interval_mct;
//  if (EXPECT_FALSE(wakeup <= now))
//    interval_mct = 1;
//  else
//    interval_mct = us_to_mct(wakeup - now);
//
//  timers.cpu(current_cpu())->set_interval(interval_mct);
}

//------------------------------------------------------------------------------
IMPLEMENTATION [sparc && leon3 && !debug]:

IMPLEMENT inline
void
Timer::dump() {}

//------------------------------------------------------------------------------
IMPLEMENTATION [sparc && leon3 && debug]:
#include <cstdio>

IMPLEMENT inline
void
Timer::dump()
{
  Mword si = (Proc::read_alternative<Mmu::Bypass>(config()) >> Separate_irq_shift) & 0x1;

  printf("Sparc GPTIMER configuration: %d timers @ IRQ %d (%s)\n", (int)_num_timers, (int)_irq_start, si ? "separate" : "shared");

  for (unsigned i=1; i <= _num_timers; i++)
  {
    Unsigned32 ctrl = Proc::read_alternative<Mmu::Bypass>(timer_control(i));
    int en = (ctrl >> Enable_bit)      & 0x1;
    int rs = (ctrl >> Restart_bit)     & 0x1;
    int ld = (ctrl >> Load_bit)        & 0x1;
    int ie = (ctrl >> Irq_enable_bit)  & 0x1;
    int ip = (ctrl >> Irq_pending_bit) & 0x1;
    int ch = (ctrl >> Chain_bit)       & 0x1;
    int dh = (ctrl >> Debug_halt_bit)  & 0x1;

    printf("Timer Control Registers: EN RS LD IE IP CH DH\n");
    printf("Timer %d:                  %d  %d  %d  %d  %d  %d  %d\n", i, en, rs, ld, ie, ip, ch, dh);
  }

  for (unsigned i=1; i <= _num_timers; i++)
  {
    Unsigned32 val = Proc::read_alternative<Mmu::Bypass>(timer_value(i));
    Unsigned32 rel = Proc::read_alternative<Mmu::Bypass>(timer_reload(i));
    printf("Timer %d reload: %d\n", i, rel);
    printf("Timer %d value:  %d\n", i, val);
  }
}

