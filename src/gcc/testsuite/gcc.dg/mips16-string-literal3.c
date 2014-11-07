/* { dg-do compile { target mips*-sde*-* } } */
/* { dg-options "-mips16" } */
/* { dg-final { scan-assembler-not "la" } } */
/* { dg-final { scan-assembler "rdata" } } */
/* { dg-skip-if "-mno-data-in-code only test" "mips*-sde-elf" "*" "-mno-data-in-code" } */

#if defined(__mips16)

int func ()
{
  return call ("string literal");
}

static char *normal = "string literal";

#endif

/* The first reference of "string literal" goes into .rodata because of 
   -mno-data-in-code regardless.  Access to it is either with a %hi/%lo
   for -G0 or gp_rel without -G0.  */
