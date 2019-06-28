
#include "pic32c-flags.h"
#include "pic32c-arch.h"

pic32c_mcu_t pic32c_mcu_types[] =
{
#define PIC32C_MCU(NAME, ARCH, DEV_ATTRIBUTE, MACRO, FPU)\
  { NAME, ARCH, DEV_ATTRIBUTE, MACRO, FPU },
#include "pic32c-mcus.def"
#undef PIC32C_MCU
    /* End of list.  */
  { NULL, NULL, PIC32C_UNKNOWN_ARCH, NULL, NULL }
};

pic32c_arch_t pic32c_arch_types[] =
{
#define PIC32C_ARCH(NAME, ARM_VER, ARCH, FLAGS)\
    { NAME, ARM_VER, ARCH, FLAGS },
#include "pic32c-arch.def"
#undef PIC32C_ARCH
    {0, PIC32C_UNKNOWN_ARCH, NULL, 0}
};

