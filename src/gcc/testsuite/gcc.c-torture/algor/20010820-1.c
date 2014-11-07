/* -gcc -EB -mips2 -mcpu=r4000 -O2 */

static const unsigned char expected[] = 
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#if __MIPSEB__
  0x11, 0x22, 0x33, 0x44, 
  0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0x00, 0x00, 
  0x12, 0x34, 0x56, 0x78,
  0xbb, 0xcc, 0xdd, 0xee,
#else
  0x11, 0x22, 0x44, 0x33, 
  0x66, 0x55, 0x88, 0x77,
  0x99, 0xaa, 0x00, 0x00, 
  0x78, 0x56, 0x34, 0x12,
  0xee, 0xdd, 0xcc, 0xbb,
#endif
  0x00, 0x00, 0x00, 0x00
};

static unsigned char result[38];

void  NET_IP_send(unsigned int dst_adr,  /* destination ip address (BE) */
                  unsigned char *data)   /* pointer to start of frame   */
{
    unsigned int   our_ip_adr = 0x12345678;
    unsigned char *p ;
    unsigned char  tmp1 ;
    unsigned short tmp2 ;
    unsigned int   tmp4 ;

    /* Fill-in IP header of frame */
    p = (data + 14) ;
 
    tmp1 = 0x11; ;
    *p++ = *(unsigned char*)&tmp1;        /* Fill-in version+header length  */
 
    tmp1 = 0x22;
    *p++ = *(unsigned char*)&tmp1;        /* Fill-in 'type-of-service'      */
 
    tmp2 = 0x3344;
    *p++ = * (unsigned char*)&tmp2;       /* Fill-in 'total-length'         */
    *p++ = *((unsigned char*)&tmp2+1);
 
    tmp2 = 0x5566;
    *p++ = * (unsigned char*)&tmp2;       /* Fill-in 'identification'       */
    *p++ = *((unsigned char*)&tmp2+1);
 
    tmp2 = 0x7788;
    *p++ = * (unsigned char*)&tmp2;       /* Fill-in 'fragment' (not used)  */
    *p++ = *((unsigned char*)&tmp2+1);
 
    tmp1 = 0x99;
    *p++ = *(unsigned char*)&tmp1;        /* Fill-in 'time-to-live'         */
 
    tmp1 = 0xaa;
    *p++ = *(unsigned char*)&tmp1;        /* Fill-in 'protocol'             */
 
    tmp2 = 0 ;
    *p++ = * (unsigned char*)&tmp2;       /* Fill-in 'checksum' (temp = 0)  */
    *p++ = *((unsigned char*)&tmp2+1);
 
    tmp4 = our_ip_adr ;  /* ASSIGNMENT OF tmp4 PUSHED TOO FAR INENDIAN-BIG */
    *p++ = * (unsigned char*)&tmp4;       /* Fill-in'source-IP-adr'        */
    *p++ = *((unsigned char*)&tmp4+1);
    *p++ = *((unsigned char*)&tmp4+2);
    *p++ = *((unsigned char*)&tmp4+3);
 
    tmp4 = dst_adr ;
    *p++ = * (unsigned char*)&tmp4;       /* Fill-in 'destination-IP-adr'   */
    *p++ = *((unsigned char*)&tmp4+1);
    *p++ = *((unsigned char*)&tmp4+2);
    *p++ = *((unsigned char*)&tmp4+3);
}

int main ()
{
  NET_IP_send(0xbbccddee, result);
  if (memcmp (expected, result, sizeof(expected)) != 0)
    abort ();
  exit (0);
}
