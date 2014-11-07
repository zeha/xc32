/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-mips16 -Os" } */
/* { dg-skip-if "-mips16 && -mips64 not supported" "mips*-sde-elf" "-mips64" "" } */

/* See detail of this test in UK bugzilla 405.  */

int main (int a, int b)
{
  return (a < b) % (a == b);
}
