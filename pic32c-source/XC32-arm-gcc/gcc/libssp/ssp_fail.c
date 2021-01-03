#ifndef __DISABLE_WARNINGS__
char __attribute__((section(".gnu.warning.__stack_chk_fail")))
__warning__stack_chk_fail[] = "Linking stub __stack_chk_fail(). Hint: It is better to provide an app-specific implementation of void __stack_chk_fail (void)";
#endif

void
__attribute__((weak,noreturn)) __stack_chk_fail (void)
{
    //\ XC32 implementation
    while(1)
    {
        __builtin_software_breakpoint();
    }
}

#ifndef __DISABLE_WARNINGS__
char __attribute__((section(".gnu.warning.__chk_fail")))
__warning__chk_fail[] = "Linking stub __chk_fail(). Hint: It is better to provide an app-specific implementation of void __chk_fail (void)";
#endif

void
__attribute__((weak,noreturn)) __chk_fail (void)
{
    //\ XC32 implementation
    while(1)
    {
        __builtin_software_breakpoint();
    }
}

#ifdef HAVE_HIDDEN_VISIBILITY
void
__attribute__((visibility ("hidden")))
__stack_chk_fail_local (void)
{
  __stack_chk_fail ();
}
#endif
