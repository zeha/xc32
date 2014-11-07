/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-Os" } */
/* { dg-final { switch -regexp [board_info [target_info name] multilib_flags] mips(16|64) {} default { scan-assembler "\tmsubu" } } } */

unsigned long long test (unsigned int a, 
			 unsigned int b, 
			 unsigned long long c)
{
  return c - (unsigned long long)a * (unsigned long long)b;
}
