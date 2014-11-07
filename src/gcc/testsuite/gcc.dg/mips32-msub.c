/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-Os" } */
/* { dg-final { switch -regexp [board_info [target_info name] multilib_flags] mips(16|64) {} default { scan-assembler "\tmsub" } } } */

long long test (int a, int b, long long c)
{
  return c - (long long)a * (long long)b;
}

