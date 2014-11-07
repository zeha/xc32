struct abc
{
    int a;
    int b;
    int c;
};

const struct abc abc[]  __attribute__ (( __section__ (".PREFETCH2.data"))) = 
{ { 0, 0, 0 },
  { 1, 1, 1 },
  { 2, 2, 2 }
};


main ()
{
    int i;

    for (i = 0; i <= 2; i++)
	if (abc[i].a != i)
	    abort ();

    exit (0);
}
