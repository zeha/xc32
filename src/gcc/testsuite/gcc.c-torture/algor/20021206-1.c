#include <math.h>

#define ITERATIONS 2

int iIterations;
float a0 = 1.1 ;
float a1 = 2.2 ;
float a2 = 3.3 ;
float a3 = 4.4 ;
float a4 = 5.5 ;
float a5 = 6.6 ;
float a6 = 7.7 ;
float a7 = 8.8 ;
float a8 = 9.9 ;
float a9 = 10.11 ;
float y ;
float x = 3.0 ;
 

int
test ()
{
  for (iIterations = 0 ; iIterations < ITERATIONS ; iIterations++)
    {
      y = a0 + 
	a1*x + 
	a2*(pow(x,2)) + 
	a3*(pow(x,3)) + 
	a4*(pow(x,4)) + 
	a5*(pow(x,5)) + 
	a6*(pow(x,6)) + 
	a7*(pow(x,7)) + 
	a8*(pow(x,8)) + 
	a9*(pow(x,9)) ;
    }
  return (int)y;
}


main ()
{
  if (test() == 291013)
    exit (0);
  abort ();
}
