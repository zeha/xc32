/* { dg-do run { target mips*-*-* } } */
/* { dg-options "-O2 -std=c99" } */

/* Test zero extensions, sign extensions, truncations, and jalrc on MIPS16E.
   We can test this file with the following configurations.
   1. -mips32 -mips16
   2. -mips32 -mips16e
   3. -mips64 -mips16
   4. -mips64 -mips16e
   5. -mips32
   6. -mips32r2
   7. -mips64
   8. -mips64r2
*/

#include <stdlib.h>
#include <stdio.h>

/* Zero Extend 8 bit->16 bit */
unsigned short zero_extend8_16 (unsigned char a)
{
  return a;
}

/* Zero Extend 8 bit->32 bit */
unsigned int zero_extend8_32 (unsigned char a)
{
  return a;
}

/* Zero Extend 8 bit->64 bit */
unsigned long long zero_extend8_64 (unsigned char a)
{
  return a;
}

/* Zero Extend 16 bit->32 bit */
unsigned int zero_extend16_32 (unsigned short a)
{
  return a;
}

/* Zero Extend 16 bit->64 bit */
unsigned long long zero_extend16_64 (unsigned short a)
{
  return a;
}

/* Zero Extend 32 bit->64 bit */
unsigned long long zero_extend32_64 (unsigned int a)
{
  return a;
}

/* Sign Extend 8 bit->16 bit */
short sign_extend8_16 (signed char a)
{
  return a;
}

/* Sign Extend 8 bit->32 bit */
int sign_extend8_32 (signed char a)
{
  return a;
}

/* Sign Extend 8 bit->64 bit */
long long sign_extend8_64 (signed char a)
{
  return a;
}

/* Sign Extend 16 bit->32 bit */
int sign_extend16_32 (short a)
{
  return a;
}

/* Sign Extend 16 bit->64 bit */
long long sign_extend16_64 (short a)
{
  return a;
}

/* Sign Extend 32 bit->64 bit */
long long sign_extend32_64 (int a)
{
  return a;
}

/* Truncate 16 bit->8 bit */
signed char truncate16_8 (short a)
{
  return a;
}

/* Truncate 32 bit->8 bit */
signed char truncate32_8 (int a)
{
  return a;
}

/* Truncate 64 bit->8 bit */
signed char truncate64_8 (long long a)
{
  return a;
}

/* Truncate 32 bit->16 bit */
short truncate32_16 (int a)
{
  return a;
}

/* Truncate 64 bit->16 bit */
short truncate64_16 (long long a)
{
  return a;
}

/* Truncate 64 bit->32 bit */
int truncate64_32 (long long  a)
{
  return a;
}

void (*TEST)();

void test_jalrc()
{
  printf ("Test jalrc\n");
}

int main()
{
  unsigned char uc1, uc2, uc3;
  unsigned short us1, us2, us3;
  unsigned int ui1, ui2, ui3;
  unsigned long long ull1, ull2, ull3;

  signed char c1, c2, c3;
  short s1, s2, s3;
  int i1, i2, i3;
  long long ll1, ll2, ll3;

  TEST=test_jalrc;
  (*TEST)();
  (*TEST)(); /* In MIPS16E, this function call will be changed to jalrc. */

  /* Zero Extensions */
  /* Test 1. 8->16 */
  uc1 = 0;
  us1 = zero_extend8_16 (uc1);
  us2 = 0;
  if (us1 != us2)
    abort ();

  uc1 = 255;
  us1 = zero_extend8_16 (uc1);
  us2 = 255;
  if (us1 != us2)
    abort ();

  /* Test 2. 8->32 */
  uc1 = 0;
  ui1 = zero_extend8_32 (uc1);
  ui2 = 0;
  if (ui1 != ui2)
    abort ();

  uc1 = 255;
  ui1 = zero_extend8_32 (uc1);
  ui2 = 255;
  if (ui1 != ui2)
    abort ();

  /* Test 3. 8->64 */
  uc1 = 0;
  ull1 = zero_extend8_64 (uc1);
  ull2 = 0;
  if (ull1 != ull2)
    abort ();

  uc1 = 255;
  ull1 = zero_extend8_64 (uc1);
  ull2 = 255;
  if (ull1 != ull2)
    abort ();

  /* Test 4. 16->32 */
  us1 = 0;
  ui1 = zero_extend16_32 (us1);
  ui2 = 0;
  if (ui1 != ui2)
    abort ();

  us1 = 65535;
  ui1 = zero_extend16_32 (us1);
  ui2 = 65535;
  if (ui1 != ui2)
    abort ();

  /* Test 5. 16->64 */
  us1 = 0;
  ull1 = zero_extend16_64 (us1);
  ull2 = 0;
  if (ull1 != ull2)
    abort ();

  us1 = 65535;
  ull1 = zero_extend16_64 (us1);
  ull2 = 65535;
  if (ull1 != ull2)
    abort ();

  /* Test 6. 32->64 */
  ui1 = 0;
  ull1 = zero_extend32_64 (ui1);
  ull2 = 0;
  if (ull1 != ull2)
    abort ();

  ui1 = 0xffffffff;
  ull1 = zero_extend32_64 (ui1);
  ull2 = 0xffffffff;
  if (ull1 != ull2)
    abort ();

  /* Sign Extensions */
  /* Test 1. 8->16 */
  c1 = 0;
  s1 = sign_extend8_16 (c1);
  s2 = 0;
  if (s1 != s2)
    abort ();

  c1 = -1;
  s1 = sign_extend8_16 (c1);
  s2 = 0xffff;
  if (s1 != s2)
    abort ();

  /* Test 2. 8->32 */
  c1 = 0;
  i1 = sign_extend8_32 (c1);
  i2 = 0;
  if (i1 != i2)
    abort ();

  c1 = -1;
  i1 = sign_extend8_32 (c1);
  i2 = 0xffffffff;
  if (i1 != i2)
    abort ();

  /* Test 3. 8->64 */
  c1 = 0;
  ll1 = sign_extend8_64 (c1);
  ll2 = 0;
  if (ll1 != ll2)
    abort ();

  c1 = -1;
  ll1 = sign_extend8_64 (c1);
  ll2 = 0xffffffffffffffffLL;
  if (ll1 != ll2)
    abort ();

  /* Test 4. 16->32 */
  s1 = 0;
  i1 = sign_extend16_32 (s1);
  i2 = 0;
  if (i1 != i2)
    abort ();

  s1 = -1;
  i1 = sign_extend16_32 (s1);
  i2 = 0xffffffff;
  if (i1 != i2)
    abort ();

  /* Test 5. 16->64 */
  s1 = 0;
  ll1 = sign_extend16_64 (s1);
  ll2 = 0;
  if (ll1 != ll2)
    abort ();

  s1 = -1;
  ll1 = sign_extend16_64 (s1);
  ll2 = 0xffffffffffffffffLL;
  if (ll1 != ll2)
    abort ();

  /* Test 6. 32->64 */
  i1 = 0;
  ll1 = sign_extend32_64 (i1);
  ll2 = 0;
  if (ll1 != ll2)
    abort ();

  i1 = -1;
  ll1 = sign_extend32_64 (i1);
  ll2 = 0xffffffffffffffffLL;
  if (ll1 != ll2)
    abort ();

  /* Truncations */
  /* Test 1. 16->8 */
  s1 = 0;
  c1 = truncate16_8 (s1);
  c2 = 0;
  if (c1 != c2)
    abort ();

  s1 = -1;
  c1 = truncate16_8 (s1);
  c2 = 0xff;
  if (c1 != c2)
    abort ();

  /* Test 2. 32->8 */
  i1 = 0;
  c1 = truncate32_8 (i1);
  c2 = 0;
  if (c1 != c2)
    abort ();

  i1 = -1;
  c1 = truncate32_8 (i1);
  c2 = 0xff;
  if (c1 != c2)
    abort ();

  /* Test 3. 64->8 */
  ll1 = 0;
  c1 = truncate64_8 (ll1);
  c2 = 0;
  if (c1 != c2)
    abort ();

  ll1 = -1;
  c1 = truncate64_8 (ll1);
  c2 = 0xff;
  if (c1 != c2)
    abort ();

  /* Test 4. 32->16 */
  i1 = 0;
  s1 = truncate32_16 (i1);
  s2 = 0;
  if (s1 != s2)
    abort ();

  i1 = -1;
  s1 = truncate32_16 (i1);
  s2 = 0xffff;
  if (s1 != s2)
    abort ();

  /* Test 5. 64->16 */
  ll1 = 0;
  s1 = truncate64_16 (ll1);
  s2 = 0;
  if (s1 != s2)
    abort ();

  ll1 = -1;
  s1 = truncate64_16 (ll1);
  s2 = 0xffff;
  if (s1 != s2)
    abort ();

  /* Test 6. 64->32 */
  ll1 = 0;
  i1 = truncate64_32 (ll1);
  i2 = 0;
  if (i1 != i2)
    abort ();

  ll1 = -1;
  i1 = truncate64_32 (ll1);
  i2 = 0xffffffff;
  if (i1 != i2)
    abort ();

  printf ("Tests Pass\n");
  exit (0);
}
