float f = 42.0;

double
subr2 (void *p, double d1, double d2)
{
    return d1 + d2;
}

double 
subr ()
{
    return subr2 (0, (double)f, (double)0.0);
}

main ()
{
    double res = subr ();
    if (res < 41.99 || res > 42.01)
	abort ();
    exit (0);
}
	
