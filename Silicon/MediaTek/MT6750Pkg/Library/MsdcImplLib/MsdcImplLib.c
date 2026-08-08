#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MsdcImplLib.h>

#include <Protocol/MtkGpio.h>
#include <Protocol/MtkClock.h>
#include <Protocol/MtkPmic.h>
#include <MT6750ClkEnum.h>
//
// MSDC Platform Info - MT6750/MT6755 specific
//
MSDC_PLATFORM_INFO gPlatformInfo = {
  .NumberOfHosts   = 2,
  .UseTop          = TRUE,
  .MsdcPadTuneReg  = 0xf0,
  .TuningStep     = {32, 64},
  .AsyncFifo      = TRUE,
  .BusyCheck      = TRUE,
  .StopClkFix     = TRUE,
  .EnhanceRx      = TRUE,
};

STATIC MTK_GPIO_PROTOCOL   *mGpio  = NULL;
STATIC MTK_CLOCK_PROTOCOL  *mClock = NULL;
STATIC MTK_PMIC_PROTOCOL   *mPmic  = NULL;

VOID
GetSourceClockRate (
  UINT32 Index,
  UINTN *Hz)
{
  UINT32 ClockId = (Index == 0) ? TOP_MSDC50_0_SEL : TOP_MSDC30_1_SEL;
  EFI_STATUS Status = mClock->GetFrequency(ClockId, Hz);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: GetFrequency(%d) failed! %r\n", Index, Status));
  }
}

VOID
SourceClockControl (
  UINT32 Index,
  BOOLEAN Enable)
{
  //
  // MT6750: No INFRA_FAES_FDE clock. Just control INFRA_MSDC gate directly.
  //
  UINT32 ClockId = (Index == 0) ? INFRA_MSDC0 : INFRA_MSDC1;
  EFI_STATUS Status = mClock->SetEnable(ClockId, Enable);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: SourceClockControl(%d)=%a failed! %r\n",
           Index, Enable ? "ON" : "OFF", Status));
  }
}

VOID
ClockControl (
  UINT32 Index,
  BOOLEAN Enable)
{
  EFI_STATUS Status;
  UINT32 ClockId;

  // 1) TOP MUX - select parent clock for MSDC
  ClockId = (Index == 0) ? TOP_MSDC50_0_SEL : TOP_MSDC30_1_SEL;
  Status = mClock->SetEnable(ClockId, Enable);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: TOP MUX %d %a failed! %r\n",
           Index, Enable ? "ON" : "OFF", Status));
  }

  // 2) INFRA Gate - enable/disable MSDC bus clock
  ClockId = (Index == 0) ? INFRA_MSDC0 : INFRA_MSDC1;
  Status = mClock->SetEnable(ClockId, Enable);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: INFRA gate %d %a failed! %r\n",
           Index, Enable ? "ON" : "OFF", Status));
  }
}

VOID
PowerControl (
  UINT32 Index,
  BOOLEAN Enable)
{
  EFI_STATUS Status;

  if (Index == 0 && FixedPcdGetBool(PcdStorageIsEMMC)) {
    //
    // MSDC0 = eMMC -> VEMC (VCCQ, 3.3V only on MT6351)
    //
    Status = mPmic->RegulatorSetEnable("vemc", Enable);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "MSDC: VEMC %a failed! %r\n",
             Enable ? "ON" : "OFF", Status));
    }
  } else {
    //
    // MSDC1 = SD Card -> VMCH (VCC 3.3V) + VMC (VCCQ)
    //
    Status = mPmic->RegulatorSetEnable("vmch", Enable);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "MSDC: VMCH %a failed! %r\n",
             Enable ? "ON" : "OFF", Status));
    }

    Status = mPmic->RegulatorSetEnable("vmc", Enable);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "MSDC: VMC %a failed! %r\n",
             Enable ? "ON" : "OFF", Status));
    }
  }
}

VOID
InitGpio (
  UINT32 Index)
{
  if (Index == 0 && FixedPcdGetBool(PcdStorageIsEMMC)) {
    //
    // MSDC0 = eMMC (MT6755/MT6750 pinfunc verified)
    // Mode 1 = MSDC0 function
    //
    mGpio->SetMode(178, 1); // DAT0
    mGpio->SetMode(179, 1); // CLK
    mGpio->SetMode(180, 1); // CMD
    mGpio->SetMode(181, 1); // DAT1
    mGpio->SetMode(186, 1); // DAT2
    mGpio->SetMode(187, 1); // DAT3
    mGpio->SetMode(184, 1); // DAT4
    mGpio->SetMode(182, 1); // DAT5
    mGpio->SetMode(183, 1); // DAT6
    mGpio->SetMode(188, 1); // DAT7
    mGpio->SetMode(185, 1); // RSTB (eMMC hardware reset)
    mGpio->SetMode(189, 1); // DSL (data strobe for HS400)
  } else {
    //
    // MSDC1 = SD Card (MT6755/MT6750 pinfunc verified)
    //
    mGpio->SetMode(30, 1);  // CLK
    mGpio->SetMode(32, 1);  // CMD
    mGpio->SetMode(33, 1);  // DAT0
    mGpio->SetMode(35, 1);  // DAT1
    mGpio->SetMode(34, 1);  // DAT2  (NOT 163!)
    mGpio->SetMode(31, 1);  // DAT3  (NOT 164!)
  }
}

EFI_STATUS EFIAPI
MsdcLibConstructor (VOID)
{
  EFI_STATUS Status;

  // Locate protocols
  Status = gBS->LocateProtocol(&gMediaTekGpioProtocolGuid, NULL, (VOID **)&mGpio);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: Failed to locate GPIO protocol! %r\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol(&gMediaTekClockProtocolGuid, NULL, (VOID **)&mClock);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: Failed to locate Clock protocol! %r\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol(&gMediaTekPmicProtocolGuid, NULL, (VOID **)&mPmic);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "MSDC: Failed to locate PMIC protocol! %r\n", Status));
    return Status;
  }

  //
  // Configure SD card LDO voltages
  // VMCH = 3.3V (SD card main power)
  // VMC  = 3.3V (SD card I/O, will be switched to 1.8V by MSDC driver if UHS supported)
  //
  mPmic->RegulatorSetVoltage("vmch", 3300000);
  mPmic->RegulatorSetVoltage("vmc",  3300000);

  //
  // Configure eMMC LDO voltage
  // VEMC = 3.3V (MT6351 VEMC min is 2.9V, so only 3.3V mode is possible)
  //
  mPmic->RegulatorSetVoltage("vemc", 3300000);

  DEBUG((DEBUG_INFO, "MSDC: MsdcLibConstructor done.\n"));
  return EFI_SUCCESS;
}
