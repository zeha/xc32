/* { dg-do run { target mips*-*-* } } */

#if (__mips == 32)
__attribute__((nomips16))
static
long m32_static (double x)
{
  return (double) (long) x;
}

__attribute__((nomips16))
long
m32 (double f)
{
  return f;
}

__attribute__((mips16))
static
long m16_static (double x)
{
  return (double) (long) x;
}

__attribute__((mips16))
long
m16 (double f)
{
  return f;
}

__attribute__((nomips16))
double
f32 (long i)
{
  if (m32 (i) != m32_static (i))
    abort ();
  if (m16 (i) != m16_static (i))
    abort ();
  return m32_static (i);
}

__attribute__((mips16))
double
f16 (long i)
{
  if (m32 (i) != m32_static (i))
    abort ();
  if (m16 (i) != m16_static (i))
    abort ();
  return m16_static (i);
}
#endif

int main ()
{
#if (__mips == 32)
  if (f32 (123456789L) != m32_static (123456789L))
    abort ();
  if (f16 (123456789L) != m16_static (123456789L))
    abort ();
#endif
  exit (0);
}
