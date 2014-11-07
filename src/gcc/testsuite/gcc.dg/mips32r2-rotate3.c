/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-O2 -finline-functions" } */
/* { dg-final { scan-assembler-not "srl" } } */
/* { dg-final { scan-assembler-not "sll" } } */

#if (!defined(__mips16) && \
     ((__mips == 32 && __mips_isa_rev == 2) \
      || (__mips == 64  && __mips_isa_rev == 2)))

unsigned func2 (unsigned a, unsigned b)
{
  return (a >> (32 - b)) | (a << b);
}

/* 2 RTLs will be generated.
  reg x (neg reg b)
  reg x (rotatert (reg a reg x))

The inlining makes b = 17, after const prop we have
  reg x (rotatert (reg a const_int -17)).  
  
  We should scan for "ror $, $, -17" */

int func1 (unsigned a)
{
  return func2 (a, 17);
}

#endif
