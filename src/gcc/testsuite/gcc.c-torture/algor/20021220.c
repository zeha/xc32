struct zero {
    int a;
    int b[0];
    int c;
};

struct zeroonly {
    int b[0];
};

union zerounion {
    int a;
    int b[0];
};

struct flexok {
    int a;
    int c;
    int b[];
};

struct flexbad {
    int a;
    int c;
    int b[]; /* error: not last in structure */
    int d;
};

struct flexbadonly {
    int b[]; /* error: only member of structure */
};

union flexbadunion {
    int a;
    int b[]; /* error: in union */
};

main ()
{
    return 0;
}
