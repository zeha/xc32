#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Portable random number generator defined below.  Taken from ANSI C by  *
 * K & R.  Not a great generator, but fast, and good enough for my needs. */

#define IA 1103515245u
#define IC 12345u
#define IM 2147483648u
#define CHECK_RAND 

static unsigned int current_random = 0;

int my_irand (int imax)
{

    /* Creates a random integer between 0 and imax, inclusive.  i.e. [0..imax] */ 

    int ival;

    /* current_random = (current_random * IA + IC) % IM; */
    current_random = current_random * IA + IC;  /* Use overflow to wrap */
    ival = current_random & (IM - 1);  /* Modulus */
    ival = (int) ((float) ival * (float) (imax + 0.999) / (float) IM);

    if ((ival < 0) || (ival > imax)) {
        printf("Bad value in my_irand, imax = %d  ival = %d\n",imax,ival);
        exit(1);
    }

    return(ival);
}

int main(int argc, char **argv)
{
    extern void check (int, int);
    int i;

    for (i = 0; i < 10; i++) {
        check (i, my_irand(288));
    }
    return 0;
}

void
check (int i, int rval)
{
  static int expect[] = {0, 189, 88, 195, 30, 149, 141, 174, 106, 74};
  if (rval != expect[i])
    abort ();
}

