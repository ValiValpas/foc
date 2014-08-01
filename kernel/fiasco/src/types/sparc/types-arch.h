#ifndef TYPES_ARCH_H__
#define TYPES_ARCH_H__

#define L4_PTR_ARG(a) ((Address)(a)) 

#define L4_PTR_FMT             "%08lx"
#define L4_MWORD_FMT           "%08lx"
#define L4_X64_FMT             "%016llx"
#define L4_ADDR_INPUT_FMT      "%8x"
#define L4_FRAME_INPUT_FMT     "%5x"

/// standard fixed-width types
typedef unsigned char          Unsigned8;
typedef signed char            Signed8;
typedef unsigned short         Unsigned16;
typedef signed short           Signed16;
typedef unsigned int           Unsigned32;
typedef signed int             Signed32;
typedef unsigned long long int Unsigned64;
typedef signed long long int   Signed64;

/// machine word
typedef signed long int        Smword;
typedef unsigned long int      Mword;
typedef unsigned long long int Dword;

enum {
  MWORD_BITS = 32,
  ARCH_PAGE_SHIFT = 12,
};

/// virtual address
typedef unsigned long int      Address;
/// physical address
typedef unsigned long long int Paddress;

enum Address_vals
#ifdef __cplusplus
: Address
#endif
{
  Invalid_address = ~0UL
};

enum Paddress_vals
#ifdef __cplusplus
: Paddress
#endif
{
  Invalid_paddress = ~0ULL
};

typedef Unsigned64 Cpu_time;


#endif
