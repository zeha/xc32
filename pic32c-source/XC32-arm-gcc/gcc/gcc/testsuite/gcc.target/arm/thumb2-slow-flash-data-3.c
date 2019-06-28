/* { dg-do compile } */
/* { dg-require-effective-target arm_cortex_m } */
/* { dg-require-effective-target arm_thumb2_ok } */
/* { dg-skip-if "do not override -mfloat-abi" { *-*-* } { "-mfloat-abi=*" } { "-mfloat-abi=hard" } } */
/* { dg-options "-mthumb -mfloat-abi=hard -mslow-flash-data" } */

/* From PR71607 */

float b;
void fn1 ();

float
fn2 ()
{
  return 1.1f;
}

void
fn3 ()
{
  float a[2];
  a[1] = b;
  fn1 (a);
}

/* { dg-final { scan-assembler-not "\\.(float|l\\?double|\d?byte|short|int|long|quad|word)\\s+\[^.\]" } } */
