/* tests strict-aliasing bug */

typedef struct rtx_def
{
  short code;
  int dummy;
} rtx;


void put_code (rtx *rt, short code)
{
 memset (rt, 0, sizeof *rt);
 rt->code = code;
}

main ()
{
  rtx x;
  put_code (&x, 5);
  if (x.code != 5)
    abort ();
  exit (0);
}
