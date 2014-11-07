unsigned char f(void)
{
 int i;
 unsigned char bytes[16];
 bytes[15] = 0xff;
 i=0; while(i < 16)
  {
     int tmp = i;
     i = ((i + 1) << 16) >> 16;
     bytes[tmp]=0;
  }
 return bytes[15];
 }

main ()
{
    if (f () != 0x00)
	abort();
    exit (0);
}
