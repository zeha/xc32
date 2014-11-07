/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O -msmartmips" } */
/* { dg-skip-if "-msmartmips && -mips16 not supported" "mips*-sde-elf" "-mips16*" "" } */

int scaled_indexed_word_load (int a[], int b)
{
  return a[b];
}
/* { dg-final { scan-assembler "lwxs" } } */


int rotate_right (unsigned a, unsigned s)
{
  return (a << s) | (a >> (32 - s));
}
/* { dg-final { scan-assembler "ror" } } */
