#!/bin/ksh
for p in `cat smart-io.pfx` " "; do
  if [[ "x${p}x" == "x x" ]]; then
    p=""
  fi
  file="${p}printf-chk.c"
  cat <<EOF >$file
/* 
   smart-io presence test: $file
   Invoke all variants of smartio function _${p}printf and check
   for compilation or link errors.
*/
#define PREFIX $p
#define ROOT printf
#include "harness.h"
EOF
  for s in `cat smart-io.sfx`; do
    echo "DECL($s);" >>$file
  done
  cat <<EOF2 >>$file
static void test (void)
{
  FN_ARG_DECL;
EOF2
  for s in `cat smart-io.sfx`; do
    # check for components of suffix and build argument list 
    fmts=""
    args=""
    for c in c s eE fF gG; do
      if echo $s | grep $c >/dev/null 2>&1; then
        fmts="$fmts FMT_$c"
        args="$args ARG_$c"
      fi
    done
    echo "  LIBFUNC($s, _quot($fmts) _eval3($args));" >>$file
  done
  echo "}" >>$file
  echo "" >>$file

  for s in `cat smart-io.sfx`; do
    file="${p}printf-${s}-cmp.c"
    cat <<EOF3 >$file
/* smart-io output test: $file 
   Invoke stdio function with appropriate arguments. To be compiled
   with and without -mno-smart-io. 
*/
#define PREFIX $p
#define ROOT printf
#include "harness.h"
static void test (void)
{
  FN_ARG_DECL;
EOF3
    # check for components of suffix and build argument list 
    fmts=""
    args=""
    for c in c s eE fF gG; do
      if echo $s | grep $c >/dev/null 2>&1; then
        fmts="$fmts FMT_$c"
        args="$args ARG_$c"
      fi
    done
    echo "  FUNC(_quot($fmts) _eval3($args));" >>$file
    echo "}" >>$file
    echo "" >>$file
  done
  for s in `cat smart-io.sfx`; do
    # s-only forms may optimize away to puts..
    if [[ "$s" == "s" ]]; then
      continue
    fi
    file="${p}printf-${s}-opt.c"
    cat <<EOF4 >$file
/* 
   smart-io merge test: $file
   Invoke standard IO function ${p}printf and ensure the mangled symbol is generated.
   No checking of the actual mangling vs. arguments is performed.
*/
/* { dg-do compile } */
/* { dg-additional-options "-msmart-io=1" } */
#define PREFIX $p
#define ROOT printf
#include "harness.h"
static void test (void)
{
  FN_ARG_DECL;
EOF4
    # check for components of suffix and build argument list 
    fmts=""
    args=""
    for c in c s eE fF gG; do
      if echo $s | grep $c >/dev/null 2>&1; then
        fmts="$fmts FMT_$c"
        args="$args ARG_$c"
      fi
    done
    echo "  FUNC(_quot($fmts) _eval3($args));" >>$file
    echo "}" >>$file
    echo "" >>$file
    cat <<EOF5 >>$file
/* { dg-final { scan-assembler "__${p}printf_\[a-zA-Z\]*" } } */
EOF5
  done
done

