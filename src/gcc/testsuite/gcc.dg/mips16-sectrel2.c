/* { dg-do run { target mips*-*-* } } */
/* { dg-options "-funit-at-a-time -fsection-relative-addressing" } */

#include <stdio.h>

/* This will put 3 words in the .data section and throw off the section
   relative base to start of effectively at (0xc). */
static char* list[] = {
  "String A",
  "String B",
  0
};

int func1 ()
{
  return (int)list[0] + 1;
}

long long func2 ()
{
  static int int2 = 3;
  static long long longlong1 = 2;

  return int2 + longlong1;
}

int main ()
{
#if defined(__mips16)
  if (func2 () != 5)
    abort ();
#else
  printf ("Unsupported test.\n");
#endif
  exit (0);
}
