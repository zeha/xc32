/* { dg-do compile { target mips*-sde-* } } */
/* { dg-options "-O2" } */
/* { dg-final { if  [string match "*mips16*" [board_info [target_info name] multilib_flags] ] { scan-assembler-not "seh" } } } */
/* { dg-final { if ![string match "*mips16*" [board_info [target_info name] multilib_flags] ] { scan-assembler-not "sra"  } } } */

/* In MIPS64, the return type is forcing the compiler to do a truncate to 
   HI than sign_extend to SI, which the compiler later generates 
   dsll and dsra by 48. 

   compiler initially generates:
     (set (reg:HI XXX) (truncate:HI (reg:DI YYY)))
     (set (reg:SI ZZZ) (sign_extend:SI (reg:HI XXX)))

   which "combine" changes it to:
     (set (reg:SI ZZZ) (sign_extend:SI (truncate:HI (reg:DI YYY))))

   then a split happends after reload:
     (set (reg:DI X) (ashift:DI (reg:DI X) (const_int 48)))
     (Set (reg:SI X) (truncate:SI (ashiftrt:DI (reg:DI X) (const_int 48))))
*/
/* { dg-skip-if "Unsupported test" "mips*-sde-elf" "-mips64" "" } */

typedef union {
  unsigned char  ui8;
  signed char    i8;
  unsigned short ui16;
  signed short   i16;
  unsigned int   ui32;
  signed int     i32;
} __attribute__((packed)) ulint_t;

signed int
i16 (ulint_t *ip)
{
  return ip->i16;
}
