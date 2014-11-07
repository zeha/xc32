struct s {
    int val;
};

extern struct s s;		/* declare as extern */

static int func1 (void)
{
    return s.val;
}

struct s s = {42};		/* declare as global, initialised */

static int func2 (void)
{
    return s.val;
}

main ()
{
    if (func1() + func2() != 84)
	abort ();
    exit (0);
}
