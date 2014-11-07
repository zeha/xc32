// { dg-prune-output "In file included from" } 
// { dg-prune-output "#include_next" } 

#include <limits.h>
enum test {
  z = 0,
  c = UINT_MAX + 1LL
} x = z;

int main() {
  return x != z;
}
