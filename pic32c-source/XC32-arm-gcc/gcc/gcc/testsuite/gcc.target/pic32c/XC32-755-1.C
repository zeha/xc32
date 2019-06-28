/* Test static union inside template function */
/* { dg-do compile } */

template<typename T>
T
f (T x)
{
  static union 
    {
      T i;
    };
  T j = i;
  i = x;
  return j;
}


int
x (int i)
{
  return f<int> (i);
}

int main (void)
{

  return 0;
}