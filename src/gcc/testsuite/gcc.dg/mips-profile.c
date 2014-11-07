/* Check that -pg profiling at least compiles correctly */
/* { dg-options "-O2 -pg -mtune=24k" } */
/* { dg-do compile { target mips*-*-elf* } } */
/* { dg-skip-if "function sections are incompatible with profiling" "mips*-sde-elf" "-ffunction-sections" ""  } */

int             Bool_Glob;
char            Ch_Glob;

int f ()
{
  Ch_Glob = 'A';
  Bool_Glob = 0;
}

/* { dg-final { scan-assembler {_mcount} } } */
