typedef int v2q15 __attribute__((mode(V2HI)));

int main ()
{
    v2q15 u= {1,2};
    v2q15 v= {1,2};

    if ((int)u == (int)v)
        exit (0);
    else
        abort ();
}
