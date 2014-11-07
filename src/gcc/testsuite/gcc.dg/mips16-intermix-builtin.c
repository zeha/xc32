/* { dg-do run { target mips*-*-* } } */
/* { dg-options "-mips16" } */

/* Test calling a builtin function from a non-mips16 caller with 
   -mips16 in the command options.  */

#include <math.h>

__attribute__((nomips16))
main()
{
  volatile double a;
  double c;
  a = 32.0;
  c = pow(a, 1.0/3.0);
  if (c + 0.1 > 3.174802
      && c - 0.1 < 3.174802)
    exit (0);
  else
    abort ();
}
