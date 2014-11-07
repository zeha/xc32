/* { dg-options "" } */

typedef int v2q15 __attribute__((mode(V2HI)));

int main ()
{
    if (((int)((v2q15){1,1})) != 0x00010001)
        abort ();
    else
        exit (0);
}
