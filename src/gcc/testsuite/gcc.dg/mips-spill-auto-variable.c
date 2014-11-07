/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O" } */

typedef unsigned short int WORD;            // 16-bit
typedef unsigned char BYTE;
typedef union _WORD_VAL
{
    WORD Val;
    struct
    {
        BYTE LSB;
        BYTE MSB;
    } byte;
    BYTE v[2];
} WORD_VAL;


WORD_VAL func ()
{
  WORD_VAL tempVal;
  tempVal.byte.LSB = MPFSGet();
  tempVal.byte.MSB = MPFSGet();
  return tempVal;
}

/* A combination of store-bytes and load-half should be generated for both 
   mips32/64 and mips16 code, and ins/ext for mips32/64 r2 code.  */
/* { dg-final { scan-assembler "(sb|ins)" } } */
/* { dg-final { scan-assembler "(lhu|ext)" } } */
