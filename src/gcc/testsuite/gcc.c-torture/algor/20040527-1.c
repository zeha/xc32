/* combine bug */

int glob;

__attribute__((noinline))
int foo (int value)
{
  return value;
}

__attribute__((noinline))
int bar (int value)
{
  /* make bar do something with side-effects */
  glob = value;
}

__attribute__((noinline)) 
test (int a, int b)
{
  int t = a * b;
  int v;
  v = foo (t);
  (void) bar (t);
  return v;
}

int main(int argc, char **argv)
{
  if (test (42, 10) != 420)
    abort ();
  return 0;
}
