struct bfd {
  const char * name;
  int foo;
}; 

typedef struct bfd bfd_target;

const bfd_target ihex_vec = { "ihex", 22 };

main ()
{
  exit (0);
}
