#include <Library/PmicWrapperImplLib.h>

MTK_PMIC_WRAPPER_PLATFORM_INFO gPlatformInfo = {
  .RegMap = {
    [PmicWrapperInitDone2]   = 0x9C,   // PMIC_WRAP_INIT_DONE2 (AP SW init done)
    [PmicWrapperWacs2Cmd]    = 0xA0,   // PMIC_WRAP_WACS2_CMD
    [PmicWrapperWacs2RData]  = 0xA4,   // PMIC_WRAP_WACS2_RDATA
    [PmicWrapperWacs2VldClr] = 0xA8,   // PMIC_WRAP_WACS2_VLDCLR
  },
  .ArbCapabilities = FALSE,
};
