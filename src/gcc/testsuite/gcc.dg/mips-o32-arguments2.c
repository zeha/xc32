/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O" } */

extern void foo2 (double, double, double);

void bar (void)
{
  foo2 (1.0, 2.0, 3.0);
}

/* 1st parameter in $f12. ($4/$5 if soft-float)
/* { dg-final { if  [string match "*soft-float*" [board_info [target_info name] multilib_flags] ] { scan-assembler "\$5*" } } } */
/* { dg-final { if ![string match "*soft-float*" [board_info [target_info name] multilib_flags] ] { scan-assembler "f12"  } } } */

/* 2nd parameter in $f14, except for -mips64. ($6/$7 if soft-float) */
/* { dg-final { if  [string match "sde-msim-*4k*c*" [target_info name] ] { scan-assembler "\$7*" } } } */
/* { dg-final { if  [string match "sde-msim-24kf-*" [target_info name] ] { scan-assembler "f14"  } } } */
/* { dg-final { if  [string match "*mfp64*"  [target_info name] ] { scan-assembler "f14" } } } */
/* { dg-final { if  [string match "*mips16*" [target_info name] ] { scan-assembler "f14" } } } */
/* { dg-final { if  [string match "*mips64*" [target_info name] ] { scan-assembler "f13" } } } */

/* 3rd parameter on the stack 16-20($sp) for o32. */
/* { dg-final { if ![string match "*mips64*" [target_info name] ] { scan-assembler "16.*sp" } } } */
