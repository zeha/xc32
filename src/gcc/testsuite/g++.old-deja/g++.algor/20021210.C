// Build don't link: 

const unsigned char *
foo ()
{
	static const unsigned char DATA_TABLE[1] = {
	  1
	};

	static const unsigned char *POINTER_TABLE[1] = {
	  DATA_TABLE
	};

	return POINTER_TABLE[0];
}

int
main ()
{
  (void) foo ();
  return 0;
}

