/* { dg-do run { target mips*-*-* } } */
/* { dg-options "-O2 -mips3d" } */
/* { dg-prune-output "require 64-bit h/w floating point" } */
/* { dg-prune-output "Assembler messages:\nError: -mfp32 used with -mips3d" } */
/* { dg-skip-if "-mips3d && -mips16 not supported" "mips*-sde-elf" "-mips16" "" } */
/* { dg-skip-if "-mips3d needs 64-bit floating point support" "mips*-sde-elf" "-mips32*" "-mfp64" } */

#if (((__mips == 32 && __mips_isa_rev == 2) || __mips == 64) \
	&& defined(__mips_hard_float) && __mips_fpr == 64)

typedef float v2sf __attribute__ ((vector_size(8)));

static const v2sf a = { 1, 2 };
static const v2sf b = { 2, 1 };
static const v2sf c = { 2, 2 };

v2sf __attribute__((noinline))
foo (v2sf x, v2sf y)
{
  if (__builtin_mips_any_c_lt_ps (x, y))
    x = __builtin_mips_movt_c_lt_ps (x, y, x, y);
  return x;
}

int main()
{
  if (!__builtin_mips_all_c_eq_ps (foo (a, b), c))
    abort ();

  printf ("Test Passes\n");
  exit (0);
}

#else

int main ()
{
  printf ("Unsupported test\n");
  exit (0);
}

#endif
