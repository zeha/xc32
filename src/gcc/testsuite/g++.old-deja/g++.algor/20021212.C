int 
bar (const int *ip)
{
  return *ip;
}

int 
foo (void)
{
  static const int i = 2;
  return bar (&i);
}

int 
frog ()
{
  static const int i = 3;
  return i;
}

int
main ()
{
  if (foo () + frog () != 5)
    return 1;
  return 0;
}
