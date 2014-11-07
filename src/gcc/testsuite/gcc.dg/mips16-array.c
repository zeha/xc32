/* { dg-do run { target mips*-*-* } } */
/* This was reported as UK Bugzilla 456 where MIPS16 generates incorrect code.  */

#include <stdio.h>

int main (int argc, char **argv)
{
  int i;
  static int A[] = {11, 22, 33, 44, 55};
  
  if ((A+1)[-1] != 11)
    abort ();
  if ((A+2)[-1] != 22)
    abort ();
  if ((A+3)[-1] != 33)
    abort ();
  if ((A+4)[-1] != 44)
    abort ();
  if ((A+5)[-1] != 55)
    abort ();

  exit (0);
}
