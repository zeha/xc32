int LIB_VERSION = 0;

int isnanf  (float __value)
{
  union {float f; unsigned int i} v;
  v.f = __value;
  return (v.i & 0x7f800000) == 0x7f800000;
}

double kernel_standard (double a, double b, int c)
{
  return -1.0;
}

float sqr (float v)
{
  return v * v;
}

float sqrf(float x)		 
{
  float z;
  z = sqr(x);
  if(LIB_VERSION == -1 || isnanf(x)) return z;
  if(x<(float)0.0)
    return (float)kernel_standard((double)x,(double)x,126);
  else
    return z;
}

main ()
{
  float v;

  v = sqrf (2.0f);
  if (v < 3.99f || v > 4.01f)
    abort ();

  v = sqrf (0.0f/0.0f);
  if (v == v)
    abort ();

  if (sqrf (-2.0f) > 0.0f)
    abort ();

  exit (0);
}
