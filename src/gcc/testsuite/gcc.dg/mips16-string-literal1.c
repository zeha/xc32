/* { dg-do compile { target mips*-sde*-* } } */
/* { dg-options "-mips16" } */
/* { dg-final { scan-assembler "rdata" } } */
/* { dg-final { scan-assembler "lw" } } */
/* { dg-skip-if "Unsupported test" "mips*-sde-elf" "-G0" "" } */

#if defined(__mips16)

static char *normal = "string literal";

int func ()
{
  return call ("string literal");
}

#endif

/* The first reference of "string literal" goes into .rodata, an indirection
   is created in the constant pool for the second reference. */
