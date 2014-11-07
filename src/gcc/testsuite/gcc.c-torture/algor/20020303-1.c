inline const char *
inline1 (int which)
{
    if (which == 0)
	return "string1";
    return "string2";
}

inline const char *
inline2 (int which)
{
    if (which == 0)
	return "string1";
    return "string2";
}

const char *
bar ()
{
    return inline1 (1);
}

main ()
{
  if (strcmp (inline1 (0), inline2 (0)) != 0)
    abort ();
  exit (0);
}
