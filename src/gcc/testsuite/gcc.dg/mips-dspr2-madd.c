/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-O2" } */
/* { dg-skip-if "DSP && -mips16 not supported" "mips*-sde-elf" "-mips16" "" } */

/* { dg-final { scan-assembler "madd" } } */
/* { dg-final { scan-assembler "ac0" } } */
/* { dg-final { scan-assembler "ac1" } } */
/* { dg-final { scan-assembler "ac2" } } */
/* { dg-final { scan-assembler "ac3" } } */

#if __mips_dsp && (_MIPS_ARCH_74KC || _MIPS_ARCH_74KF || _MIPS_ARCH_74KFX || _MIPS_ARCH_74KX)

typedef long long a64;
a64 a[4];
int b[4], c[4];

void test()
{
  /* Try to see if GCC uses ac0 ac1 ac2 ac3 for madd.  */
  a[0] += (a64) b[0] * c[0];
  a[1] += (a64) b[1] * c[1];
  a[2] += (a64) b[2] * c[2];
  a[3] += (a64) b[3] * c[3];
}

#else

/* Generate some text for scanning.  */
void test()
{
  asm volatile ("# madd");
  asm volatile ("# ac0");
  asm volatile ("# ac1");
  asm volatile ("# ac2");
  asm volatile ("# ac3");
}

#endif
