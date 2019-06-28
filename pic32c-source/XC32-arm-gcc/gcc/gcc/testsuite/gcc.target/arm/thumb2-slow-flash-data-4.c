/* { dg-do compile } */
/* { dg-require-effective-target arm_cortex_m } */
/* { dg-require-effective-target arm_thumb2_ok } */
/* { dg-require-effective-target arm_vfp_ok } */
/* { dg-skip-if "do not override -mfloat-abi" { *-*-* } { "-mfloat-abi=*" } { "-mfloat-abi=hard" } } */
/* { dg-options "-O2 -mthumb -mfloat-abi=hard -mslow-flash-data" } */

double __attribute__ ((target ("fpu=fpv5-d16")))
foo (void)
{
  return 1.0f;
}

float __attribute__ ((target ("fpu=fpv5-d16")))
bar (void)
{
  return 1.0f;
}

float __attribute__ ((target ("fpu=fpv5-sp-d16")))
baz (void)
{
  return 1.0f;
}

/* { dg-final { scan-assembler-times "#1\\.0e\\+0" 3 } } */
