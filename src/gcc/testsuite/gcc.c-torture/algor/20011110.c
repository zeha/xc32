#include <stddef.h>
#include <signal.h>

int glob = 0;

void
sighandler ()
{
  glob = 42;
}

main ()
{
#ifndef SIGNAL_SUPPRESS
  signal (SIGHUP, sighandler);
  raise (SIGHUP);
  if (glob != 42)
    abort ();
#endif
  exit (0);
}
