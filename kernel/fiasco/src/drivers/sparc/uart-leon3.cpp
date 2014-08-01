IMPLEMENTATION[uart_leon3 && libuart]:

#include "uart_leon3.h"
#include "mem_layout.h"

IMPLEMENT Address Uart::base() const { return Mem_layout::Uart_phys_base; }

IMPLEMENT int Uart::irq() const
{
  // FIXME read irq number from plug&play section
  return 2;
}

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_leon3 uart;
  return &uart;
}
