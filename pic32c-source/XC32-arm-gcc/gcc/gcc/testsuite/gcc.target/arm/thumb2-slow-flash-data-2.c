/* { dg-do compile } */
/* { dg-require-effective-target arm_cortex_m } */
/* { dg-require-effective-target arm_thumb2_ok } */
/* { dg-skip-if "do not override -mfloat-abi" { *-*-* } { "-mfloat-abi=*" } { "-mfloat-abi=hard" } } */
/* { dg-options "-O2 -mthumb -mfloat-abi=hard -mslow-flash-data" } */

float f (float);

const float max = 0.01f;

int
g (float in)
{
  if (f (in) + f (in) < max)
    return 0;
  return 1;
}

double foo (void)
{
  return 0xF1EC7A5239123AF;
}

double bar (void)
{
  return 0.0f;
}
