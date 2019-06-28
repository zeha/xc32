/* Test tcm attribute error/warning messages. */
/* { dg-do compile } */

/* Test tcm attribute error/warning messages. */
/* { dg-do compile } */

volatile int __attribute__((tcm)) g_tcm_res = 0x200;
volatile int __attribute__((tcm)) g_tcm_bss;

int __attribute__((tcm)) tcm_fn(int a)
{
	return a * a;
}

int flash_fn(int b, int x)
{
    /* { dg-warning "attribute applied to automatic" "" { target *-*-* } 18 } */
	int __attribute__((tcm))  my_local_tcm = 22222;

	int i;
	for (i = 0; i < my_local_tcm; i++)
		b *= 2;

	return b * b * b + g_tcm_res;
}
