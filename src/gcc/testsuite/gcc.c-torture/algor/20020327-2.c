#pragma pack(1)
struct foo {
  unsigned char	a;
  unsigned int	b;
  unsigned char	c;
} bar = {0x11, 0x22334455, 0x66};
#pragma pack()

main ()
{
  if (bar.a != 0x11)
    abort ();
  if (bar.b != 0x22334455)
    abort ();
  if (bar.c != 0x66)
    abort ();
  exit (0);
}
