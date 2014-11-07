/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O" } */

/* FIXME: -mips64 is not yet supported for this test. */
/* { dg-skip-if "Unsupported test" "mips*-sde-elf" "-mips64" "" } */

typedef char v4i8 __attribute__((vector_size(4)));
typedef union {v4i8 a;char b[4];} v4i8_union;
typedef char v8i8 __attribute__((vector_size(8)));
typedef union {v8i8 a;char b[8];} v8i8_union;
typedef short v2q15 __attribute__((vector_size(4)));
typedef union {v2q15 a;short b[2];} v2q15_union;

v2q15 test(short a, short b)
{
  v2q15_union temp;
  temp.b[0] = a;
  temp.b[1] = b;
  return temp.a;
}
v4i8 test2(char a, char b, char c, char d)
{
  v4i8_union temp;
  temp.b[0] = a;
  temp.b[1] = b;
  temp.b[2] = c;
  temp.b[3] = d;
  return temp.a;
}
v4i8 test3(int a, int b, int c, int d)
{
  v4i8_union temp;
  temp.b[0] = a;
  temp.b[1] = b;
  temp.b[2] = c;
  temp.b[3] = d;
  return temp.a;
}
v8i8 test4(int a, int b, int c, int d)
{
  v8i8_union temp;
  temp.b[4] = a;
  temp.b[5] = b;
  temp.b[6] = c;
  temp.b[7] = d;
  return temp.a;
}

typedef union {
  struct {
    char a;
    char b[2];
    union {
      int c;
      short d[2];
    } u;
  } s;
  long long ll;
} big;

long long test5 (int a1, int a2, int a3, int a4)
{
  big b;
  b.s.a = a1;
  b.s.b[0] = a2;
  b.s.b[1] = a3;
  b.s.u.c = a4;
  return b.ll;
}

/* A combination of store-bytes and load-half should be generated for both 
   mips32/64 and mips16 code, and ins/ext for mips32/64 r2 code.  */
/* { dg-final { scan-assembler "(sb|ins)" } } */
/* { dg-final { scan-assembler "(lhu|ext)" } } */
