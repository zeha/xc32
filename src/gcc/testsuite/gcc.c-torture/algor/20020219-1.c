int foo (unsigned long long a, unsigned long long b)
{
  int as, bs;
  as = a >> 63;
  bs = b >> 63;
  if (as != bs)
    return as || !(b << 1);
  return (a == b) || as;
}

main ()
{
  if (foo (0, 0xffffffffffffffffULL) != 0)
    abort ();
  exit (0);
}
