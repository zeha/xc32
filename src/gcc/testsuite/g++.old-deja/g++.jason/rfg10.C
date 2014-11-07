// { dg-do assemble  }
// { dg-prune-output "In file included from" } 
// { dg-prune-output "#include_next" } 

// Bug: g++ doesn't notice the overflow in the enum values.

#include <limits.h>
 
enum COLOR
{
    red,
    green = ULONG_MAX, blue
};				// { dg-error "" } enum overflow
