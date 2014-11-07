typedef int v2q15 __attribute__((mode(V2HI)));

int main ()
{
    v2q15 u= {1,2};
    if ((int)u == 0)
        abort ();
    else
        exit (0);
}
