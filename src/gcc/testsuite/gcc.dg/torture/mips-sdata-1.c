/* Check that sdata-accesses are applied regardless of size or ABI.  */
/* { dg-options "-mexplicit-relocs -G8" } */
/* { dg-do compile { target mips*-*-elf* } } */
/* { dg-skip-if  "GP rel check not present when compile with -G0" "mips*-sde-elf" "-G0" "" } */

struct s { int x[4]; };
struct s my_struct __attribute__((__section__(".sdata")));

int f() { return my_struct.x[0]; }

/* { dg-final { scan-assembler {gp_?rel\(my_struct} } } */
