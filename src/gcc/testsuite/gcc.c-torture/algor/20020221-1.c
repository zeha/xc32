/* Compiler incorrectly generates an SNaN when folding the constant
   expression 0.0/0.0. It should generate a QNaN, matching the MIPS
   Architecture specification; SNaNs are never produced as arithmetic
   result values. */

compute (float *a, float *b)
{
   float zero=0.0;
   *a = 0.0/0.0;
   *b = zero/zero;
}

main ()
{
  float a, b;
  compute (&a, &b);
  if (*(unsigned int *)&a != *(unsigned int *)&b)
    abort ();
  exit (0);
}
