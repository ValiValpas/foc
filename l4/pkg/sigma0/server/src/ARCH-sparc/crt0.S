/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

.section .text.init,#alloc,#execinstr
.type _start,#function
.globl _start
_start:
  sethi %hi(crt0_stack_init), %sp
  or    %sp, %lo(crt0_stack_init), %sp
  ba   init
  nop

.LCstack: .word crt0_stack_high
.LCexit:  .word _exit

.section ".bss"

	.align	8
	.global crt0_stack_low
crt0_stack_low:
	.space	1952
	.global crt0_stack_init
crt0_stack_init:
  .space  96
	.global crt0_stack_high
crt0_stack_high:
