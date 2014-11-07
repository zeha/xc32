static double arr[3] = {
  1.0, 2.0, 3.0
};

void
test (double *ap, int idx, double val)
{
  if (val != ap[idx])
    abort ();
}

void
set (double *ap, int idx, double val)
{
  ap[idx] = val;
}

void
clear (double *ap, int idx)
{
  ap[idx] = 0.0;
}

void
run (double *ap)
{
  int idx;
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, ap[idx]);
  for (idx = 0; idx < 3; idx++)
    set (ap, idx, 4.0);
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, 4.0);
  for (idx = 0; idx < 3; idx++)
    clear (ap, idx);
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, 0.0);
}

int
main ()
{
  run (arr);
  exit (0);
}

