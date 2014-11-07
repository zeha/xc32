/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-O2" } */
/* { dg-final { scan-assembler-not "srl" } } */
/* { dg-final { scan-assembler-not "sll" } } */

#if (!defined(__mips16) && \
     ((__mips == 32 && __mips_isa_rev == 2) \
      || (__mips == 64  && __mips_isa_rev == 2)))

unsigned func2 (unsigned a, unsigned b, unsigned s)
{
  return (a >> (32 - s)) | (a << s);
}

#endif
