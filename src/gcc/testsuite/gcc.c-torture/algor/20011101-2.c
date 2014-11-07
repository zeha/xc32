static float arr[3] = {
  1.0, 2.0, 3.0
};

void
test (float *ap, int idx, float val)
{
  if (val != ap[idx])
    abort ();
}

void
set (float *ap, int idx, float val)
{
  ap[idx] = val;
}

void
clear (float *ap, int idx)
{
  ap[idx] = 0.0;
}

void
run (float *ap)
{
  int idx;
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, ap[idx]);
  for (idx = 0; idx < 3; idx++)
    set (ap, idx, 4.0f);
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, 4.0f);
  for (idx = 0; idx < 3; idx++)
    clear (ap, idx);
  for (idx = 0; idx < 3; idx++)
    test (ap, idx, 0.0f);
}

int
main ()
{
  run (arr);
  exit (0);
}

