/* PR target/17565.  GCC used to put the asm into the delay slot
   of the call.  */
/* { dg-do assemble { target mips*-*-* } } */
/* { dg-options "-O" } */
int foo (int n)
{
  register int k asm ("$16") = n;
  if (k > 0)
    {
      bar ();
#if __mips16
      asm ("li %0,0x1234" : "=r" (k));
#else
      asm ("li %0,0x12345678" : "=r" (k));
#endif
    }
  return k;
}
