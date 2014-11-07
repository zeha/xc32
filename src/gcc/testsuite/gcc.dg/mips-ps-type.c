/* Test v2sf calculations */
/* { dg-do compile { target mips*-*-* } } */ 
/* { dg-options "-O2 -mhard-float -mpaired-single -S" } */
/* { dg-error "require 64-bit h/w floating point" "paired-single unsupported" { xfail "mips*-*-*" } 0 } */

/* { dg-final { scan-assembler "cvt.ps.s" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "mov.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "ldc1" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "sdc1" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "add.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "sub.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "neg.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "mul.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "madd.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "msub.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "nmadd.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "nmsub.ps" { xfail "mips*-*-*" } } } */
/* { dg-final { scan-assembler "mov(n|z).ps" { xfail "mips*-*-*" } } } */

typedef float v2sf __attribute__ ((vector_size(8)));

v2sf A = {100, 200};

/* Init from  floats */
v2sf init (float a, float b)
{
  return (v2sf) {a, b};
}

/* Move between registers */
v2sf move (v2sf a)
{
  return a;
}

/* Load from memory */
v2sf load ()
{
  return A;
}

/* Store to memory */ 
void store (v2sf a)
{
  A = a;
}

/* Add */ 
v2sf add (v2sf a, v2sf b)
{
  return a + b;
}

/* Subtract */ 
v2sf sub (v2sf a, v2sf b)
{
  return a - b;
}

/* Negate */
v2sf neg (v2sf a)
{
  return - a;
}

/* Multiply */ 
v2sf mul (v2sf a, v2sf b)
{
  return a * b;
}

/* Multiply and add */ 
v2sf madd (v2sf a, v2sf b, v2sf c)
{
  return a * b + c;
}

/* Multiply and subtract */ 
v2sf msub (v2sf a, v2sf b, v2sf c)
{
  return a * b - c;
}

/* Negate Multiply and add */ 
v2sf nmadd (v2sf a, v2sf b, v2sf c)
{
  return - (a * b + c);
}

/* Negate Multiply and subtract */ 
v2sf nmsub (v2sf a, v2sf b, v2sf c)
{
  return - (a * b - c);
}

/* Conditional Move */ 
v2sf cond_move1 (v2sf a, v2sf b, long i)
{
  if (i > 0)
    return a;
  else
    return b;
}

/* Conditional Move */ 
v2sf cond_move2 (v2sf a, v2sf b, int i)
{
  if (i > 0)
    return a;
  else
    return b;
}
