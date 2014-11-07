/* { dg-do compile { target mips*-*-* } } */
/* { dg-options "-Os" } */
/* { dg-final { scan-assembler "jal" } } */

/* This function comes from catdvi/getop1.c
   The caller (getopt_long) arg size is 20 bytes and the 
   callee (_getopt_internal) arg size is 24 bytes.  
   This shouldn't make the call to _getopt_internal to be a
   sibcall.

int getopt_long (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct option *long_options;
     int *opt_index;
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}
*/

/* Modified to catch the case for mips64 aswell. */
int getopt_long (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
     int arg1;
     int arg2;
     int arg3;
     int arg4;
     int arg5;
     int arg6;
     int arg7;
     int arg8;
     int arg9;
{
  return _getopt_internal (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 
			   arg9, 0);
}
