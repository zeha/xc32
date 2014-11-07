/* Testcase for http://gcc.gnu.org/ml/gcc-patches/2003-02/msg01017.html */
/* { dg-do link } */
/* { dg-options "-O -fpic" } */
/* { dg-warning "not supported" "PIC unsupported" { target cris-*-elf* cris-*-aout* mmix-*-* } 0 } */
/* { dg-xfail-if "PIC not supported" "mips*-*-elf" "*" "" } */

int *g;

int main (void)
{
  switch (*g)
    {
    case 0:
      {
        switch (*g)
          {
          case 0: *g = 1; break;
          case 1:
          case 2: *g = 1; break;
          case 3:
          case 4: *g = 1; break;
          }
        break;
      }
    case 1:
      {
        switch (*g)
          {
          case 0: *g = 1; break;
          case 1:
          case 2: *g = 1; break;
          case 3:
          case 4: *g = 1; break;
          }
      }
    }
  return 0;
}
