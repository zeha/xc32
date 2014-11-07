// Special g++ Options: -pg
// GROUPS passed profiling
// Skip if not native
// Used to be if not target mips-sde-elf but libraries won't work
#include <stdio.h>
int main()
{
  printf ("PASS\n");
}
