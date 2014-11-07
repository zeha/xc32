const char *error_str ()
{
  static const char *unknown = "Undefined Error";
  return unknown;
}

main ()
{
  if (strcmp (error_str(), "Undefined Error"))
    abort ();
  exit (0);
}
