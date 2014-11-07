/* { dg-do run { target mips*-*-* } } */
/* { dg-options "" } */

#if __mips_dsp

void test1 (int i)
{
  __builtin_mips_wrdsp (i, 63);
}

void test2 ()
{
  long long a = 0;
  __builtin_mips_extpdp (a, 3);
}

void test3 (int i)
{
  long long a = 0;
  __builtin_mips_extpdp (a, i);
}

void test4 ()
{
  long long a = 0;
  int i = 0;
  __builtin_mips_mthlip (a, i);
}

int main()
{
  int cntl;

  /* Test 1: wrdsp */
  __builtin_mips_wrdsp (0,63);
  test1 (63);
  cntl = __builtin_mips_rddsp (63);
  if (cntl != 63)
  {
    printf ("cntl=%d\n", cntl);
    abort ();
  }

  /* Test 2: extpdp */
  __builtin_mips_wrdsp (63,63);
  test2 ();
  cntl = __builtin_mips_rddsp (63);
  if (cntl != 59)
  {
    printf ("cntl=%d\n", cntl);
    abort ();
  }

  /* Test 3: extpdpv */
  __builtin_mips_wrdsp (63,63);
  test3 (10);
  cntl = __builtin_mips_rddsp (63);
  if (cntl != 52)
  {
    printf ("cntl=%d\n", cntl);
    abort ();
  }

  /* Test 4: mthlip */
  __builtin_mips_wrdsp (8,63);
  test4 ();
  cntl = __builtin_mips_rddsp (63);
  if (cntl != 40)
  {
    printf ("cntl=%d\n", cntl);
    abort ();
  }

  exit (0);
}

#else

int main ()
{
  printf ("Unsupported test\n");
  exit (0); 
}

#endif
