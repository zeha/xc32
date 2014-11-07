/* Build don't link: */

typedef float * vector; 
typedef vector * matrix;

/* dummy functions */
vector  vec         (int n)
{return (vector)0;}

void    freevec     (vector x)
{}

matrix  mat     (int m, int n)
{return (matrix)0;}

void  freemat   (matrix A, int m)
{}

float   tridec      (matrix T, vector b, int n, vector x)
{return 0.0;}


float spline(float a, vector x, vector y, vector c, int n, int ic) 
{
  int k = 1, i;
  float s, b0, b1, p;
  vector h, w, v;
  matrix T;

  if (ic) {
     
    h = vec (n-1);                        
    w = vec (n-1);
    v = vec (n);                        
    T = mat (3,n);                        

    for (i = 1; i < n; i++) { 
      h[i] = x[i+1] - x[i];
      if (h[i] > 0)
        w[i] = (y[i+1] - y[i])/h[i];
      else {
        w[i] = (float)0;
       }
    }
    v[1] = 0;
    v[n] = 0;
    for (i = 2; i < n; i++) 
      v[i] = 6*(w[i] - w[i-1]);

     
    for (i = 1; i < n; i++) {
      if (i > 1) {
        T[1][i] = h[i];
        T[2][i] = 2*(h[i-1] + h[i]);
      }
      if (i < n-1)
        T[3][i] = h[i];
    }
    T[2][1] = T[2][n] = 1;
    tridec (T,v,n,c);
    freemat (T,3);
    freevec (v);
    freevec (w);
    freevec (h);
  }

   
  while ((a > x[k+1]) && (k < n-1)) k++;     
  p = x[k+1] - x[k];             
  b0 = (6*y[k+1] - p*p*c[k+1])/(6*p); 
  b1 = (6*y[k]   - p*p*c[k])/(6*p); 
  s = b0*(a - x[k]) + b1*(x[k+1] - a); 
  s += (float) ((a-x[k])*(a-x[k])*(a-x[k])*c[k+1] + (x[k+1]-a)*(x[k+1]-a)*(x[k+1]-a)*c[k])/(6*p);
  return s; 

}  

main ()
{
  exit (0);
}
