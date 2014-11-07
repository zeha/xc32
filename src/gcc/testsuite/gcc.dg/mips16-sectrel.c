/* { dg-do run { target mips*-*-* } } */
/* { dg-options "-funit-at-a-time -fsection-relative-addressing" } */
/* { dg-skip-if "-mno-data-in-code not supported" "mips*-sde-elf" "-mno-data-in-code" "" } */

#include <stdio.h>

int main ()
{
#if defined(__mips16)
  static int a2, a1, a3;
  register unsigned addr1, addr2, addr3;
  printf ("Addr of a2 = 0x%x\n", &a2);
  printf ("Addr of a1 = 0x%x\n", &a1);
  printf ("Addr of a3 = 0x%x\n", &a3);
  printf ("Addr of a2 = 0x%x\n", &a2);
  printf ("Addr of a3 = 0x%x\n", &a3);
  printf ("Addr of a3 = 0x%x\n", &a3);
  addr1 = (unsigned) &a1;
  addr2 = (unsigned) &a2;
  addr3 = (unsigned) &a3;
  if (addr3 < addr2 && addr2 < addr1)
    exit (0);
  else
    abort ();
#else
  printf ("Unsupported test.\n");
  exit (0);
#endif
}

/* Alternatively get cgraph to dump out the reference counts, and check the 
   actual static counts from the dump. */
