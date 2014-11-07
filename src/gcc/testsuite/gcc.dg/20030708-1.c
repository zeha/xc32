/* PR c/11420 */
/* { dg-do link } */
/* { dg-options "-O2 -fpic" } */
/* { dg-xfail-if "PIC not supported" "mips*-*-elf" "*" "" } */

void (* volatile fn) (void);
static void foo (void)
{
}

int main (void)
{
  fn = foo;
  return 0;
}
