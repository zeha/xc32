/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-O2" } */
/* { dg-final { scan-assembler-not "srl" } } */
/* { dg-final { scan-assembler-not "sll" } } */

#if (!defined(__mips16) && \
     ((__mips == 32 && __mips_isa_rev == 2) \
      || (__mips == 64  && __mips_isa_rev == 2)))

unsigned func1 (unsigned a, unsigned b, unsigned s)
{
  return (a << s) | (a >> (32 - s));
}

#endif
