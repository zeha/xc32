/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-Os" } */
/* { dg-final { switch -regexp [target_info name] mips(16|64) {} default { scan-assembler "\tmsubu" } } } */

unsigned long long test (unsigned int a, 
			 unsigned int b, 
			 unsigned long long c)
{
  return c - (unsigned long long)a * (unsigned long long)b;
}
