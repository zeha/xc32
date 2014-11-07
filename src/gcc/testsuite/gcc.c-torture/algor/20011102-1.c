static double da[3] = {1.0, 2.0, 3.0};
static float fa[3] = {1.0, 2.0, 3.0};

void
dzero (double *dp)
{
  int i;
  for (i = 0; i < 3; i++)
    dp[i] = 0.0;
}

void
fzero (float *fp)
{
  int i;
  for (i = 0; i < 3; i++)
    fp[i] = 0.0;
}

void
dfill (double *dp, double v)
{
  int i;
  for (i = 0; i < 3; i++)
    dp[i] = v;
}

void
ffill (float *fp, float v)
{
  int i;
  for (i = 0; i < 3; i++)
    fp[i] = v;
}

int
main ()
{
  int i;

  dzero (da);
  fzero (fa);
  for (i = 0; i < 3; i++)
    if (da[i] != 0.0 || fa[i] != (float)0.0)
      abort ();

  dfill (da, 4.0);
  ffill (fa, 4.0);
  for (i = 0; i < 3; i++)
    if (da[i] != 4.0 || fa[i] != (float)4.0)
      abort ();

  exit (0);
}

