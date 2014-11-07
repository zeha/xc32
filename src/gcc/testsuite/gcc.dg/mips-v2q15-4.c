/* { dg-options "" } */

typedef int v2q15 __attribute__((mode(V2HI)));

int main ()
{
    if (((short)(int)((v2q15){0,0})) == 0)
        exit (0);
    else
        abort ();
}
