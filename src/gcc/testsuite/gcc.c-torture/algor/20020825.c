unsigned char f(void)
{
 short i;
 unsigned char bytes[16];
 bytes[15] = 0xff;
 i=0; while(i < 16)bytes[i++]=0;
 return bytes[15];
 }

main ()
{
    if (f () != 0x00)
	abort();
    exit (0);
}
