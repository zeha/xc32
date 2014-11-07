/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-std=gnu89" } */

extern void putstring (const char *);

void _doprnt ()
{
  void pad (int fill)
  {
    static char * b[] = { "                ","0000000000000000" };
    putstring(b[fill]);
  }

  putstring ("null");
  pad (1);
  pad (0);
}
