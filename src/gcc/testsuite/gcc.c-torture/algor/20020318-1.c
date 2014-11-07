char *data[] = {
    "hello",
    "there"
};

char *sdata[2] = {
    "hello",
    "there"
};

char *
subr (int a)
{
    char *str = (char *)data[1];
    
    if (a)
        str = (char *)data[0];
    
    return (str);
}

char *
ssubr (int a)
{
    char *str = (char *)sdata[1];
    
    if (a)
        str = (char *)sdata[0];
    
    return (str);
}

main ()
{
    if (strcmp (subr (0), "there") != 0
	|| strcmp (subr (1), "hello"))
	abort ();
    if (strcmp (ssubr (0), "there") != 0
	|| strcmp (ssubr (1), "hello"))
	abort ();
    exit (0);
}

