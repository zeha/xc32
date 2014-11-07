/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-Os" } */
/* { dg-final { switch -regexp [target_info name] mips(16|64) {} default { scan-assembler "\tmsub" } } } */

long long test (int a, int b, long long c)
{
  return c - (long long)a * (long long)b;
}

