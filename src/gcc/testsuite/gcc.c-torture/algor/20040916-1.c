/* see http://ukwww.mips.com/bugzilla/current/show_bug.cgi?id=156 */

double	fabs (double);

int test_func(float x)
{
  return ((int)fabs(x)) / 3;
}

int main ()
{
  if (test_func (3.0f) != 1)
    abort ();
  exit (0);
}
