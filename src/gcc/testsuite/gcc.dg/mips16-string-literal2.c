/* { dg-do compile { target mips*-sde*-* } } */
/* { dg-options "-mips16" } */
/* { dg-final { scan-assembler "la" } } */
/* { dg-final { scan-assembler-not "rdata" } } */

#if defined(__mips16)

int func ()
{
  return call ("string literal");
}

static char *normal = "string literal";

#endif

/* The first reference of "string literal" goes into the constant pool at the
   end of function, an "la" is used to fetch the value.  The second reference
   will put the constant pool address into "normal". */
