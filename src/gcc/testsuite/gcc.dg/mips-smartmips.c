/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O -msmartmips" } */

int scaled_indexed_word_load (int a[], int b)
{
  return a[b];
}
/* { dg-final { if ![string match "*mips16*" [target_info name] ] { scan-assembler "lwxs" } } } */


int rotate_right (unsigned a, unsigned s)
{
  return (a << s) | (a >> (32 - s));
}
/* { dg-final { if ![string match "*mips16*" [target_info name] ] { scan-assembler "ror" } } } */
