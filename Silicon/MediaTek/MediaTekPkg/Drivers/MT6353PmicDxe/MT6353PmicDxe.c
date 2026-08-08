#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>

#include <Protocol/MtkPmicWrapper.h>
#include <Protocol/MtkPmic.h>

//
// MT6353 PMIC Register Definitions
//
#define MT6353_TOPSTATUS        0x0028   // TOPSTATUS - Power/Home key status
#define MT6353_PWRKEY_MASK      BIT1     // PWRKEY interrupt status
#define MT6353_HOMEKEY_MASK     BIT3     // HOMEKEY interrupt status
#define MT6353_CON0_ENABLE      BIT0     // LDO CON0 enable bit

//
// MT6353 VEMC/VMCH/VMC Register Addresses
// Con0 = control (enable), AnaReg = voltage select
//
#define MT6353_VEMC_CON0        0x0A62   // VEMC_3V3_CON0
#define MT6353_VEMC_ANA         0x0AD2   // VEMC_3V3_ANA_CON0  [5:4] = vosel, 2bit

#define MT6353_VMCH_CON0        0x0ABE   // VMCH_CON0  (推测，基于 MT6351 布局)
#define MT6353_VMCH_ANA         0x0ACE   // VMCH_ANA_CON0  [5:4] = vosel, 2bit

#define MT6353_VMC_CON0         0x0AD2   // VMC_CON0  (推测)
#define MT6353_VMC_ANA          0x0AE2   // VMC_ANA_CON0  [7:4] = vosel, 4bit

#define MT6353_CHIP_ID_REG      0x0000   // SWCID

/*==================== Regulator Descriptor Types ====================*/

typedef enum {
  Ldo,
  FixedLdo,
  Buck
} MTK_REGULATOR_TYPE;

typedef struct {
  UINT32 Voltage;   // microvolts
  UINT8  Mask;      // register encoding value
} MTK_VOSEL;

typedef struct {
  UINT16 Con0Reg;
  UINT16 AnaReg;
  UINT16 VoselShift;
  UINT16 VoselMask;
  CONST MTK_VOSEL *Ranges;
  UINT16           RangesLen;
} MTK_LDO_DESC;

typedef struct {
  UINT16 Con0Reg;
  UINT32 Voltage;
} MTK_FIXED_LDO_DESC;

typedef struct {
  UINT16 Con0Reg;
  UINT16 VSelReg;
  UINT16 VSelMask;
  UINT32 Step;
  UINT32 Min;
  UINT32 Max;
} MTK_BUCK_DESC;

typedef struct {
  CONST CHAR8         *Name;
  MTK_REGULATOR_TYPE   Type;
  union {
    MTK_LDO_DESC       Ldo;
    MTK_FIXED_LDO_DESC FixedLdo;
    MTK_BUCK_DESC      Buck;
  };
} MTK_REGULATOR_DESC;

/*==================== Voltage Range Tables (MT6353) ====================*/

// MT6353 VEMC/VMCH: 2-bit encoding @ [5:4]
STATIC CONST MTK_VOSEL mVmchVemcRanges[] = {
  {2900000, 0x0},   // 00 -> 2.9V
  {3000000, 0x1},   // 01 -> 3.0V
  {3300000, 0x2},   // 10 -> 3.3V
};

// MT6353 VMC: 4-bit encoding @ [7:4]
STATIC CONST MTK_VOSEL mVmcRanges[] = {
  {1800000, 0x4},
  {2900000, 0xA},
  {3000000, 0xB},
  {3300000, 0xD},
};

/*==================== Regulator Table ====================*/

STATIC CONST MTK_REGULATOR_DESC mRegulators[] = {
  {
    .Name = "vemc",                         // Must match MsdcImplLib calls!
    .Type = Ldo,
    .Ldo = {
      .Con0Reg    = MT6353_VEMC_CON0,
      .AnaReg     = MT6353_VEMC_ANA,
      .VoselShift = 4,
      .VoselMask  = 0x3,                    // 2-bit
      .Ranges     = mVmchVemcRanges,
      .RangesLen  = ARRAY_SIZE(mVmchVemcRanges),
    },
  },
  {
    .Name = "vmch",                         // Must match MsdcImplLib calls!
    .Type = Ldo,
    .Ldo = {
      .Con0Reg    = MT6353_VMCH_CON0,
      .AnaReg     = MT6353_VMCH_ANA,
      .VoselShift = 4,
      .VoselMask  = 0x3,                    // 2-bit
      .Ranges     = mVmchVemcRanges,
      .RangesLen  = ARRAY_SIZE(mVmchVemcRanges),
    },
  },
  {
    .Name = "vmc",                          // Must match MsdcImplLib calls!
    .Type = Ldo,
    .Ldo = {
      .Con0Reg    = MT6353_VMC_CON0,
      .AnaReg     = MT6353_VMC_ANA,
      .VoselShift = 4,
      .VoselMask  = 0xF,                    // 4-bit
      .Ranges     = mVmcRanges,
      .RangesLen  = ARRAY_SIZE(mVmcRanges),
    },
  },
};

/*==================== Globals ====================*/

STATIC MTK_PMIC_WRAPPER_PROTOCOL *mPmicWrapper = NULL;

/*==================== Helpers ====================*/

STATIC
CONST
MTK_REGULATOR_DESC *
GetRegulatorByName (
  IN CONST CHAR8 *Name
  )
{
  if (Name == NULL) {
    return NULL;
  }
  for (UINTN Index = 0; Index < ARRAY_SIZE(mRegulators); Index++) {
    if (AsciiStrCmp(mRegulators[Index].Name, Name) == 0) {
      return &mRegulators[Index];
    }
  }
  return NULL;
}

/*==================== Button Functions ====================*/

VOID
PowerButtonPressed (
  OUT BOOLEAN *Pressed
  )
{
  UINT16 Value = 0;
  mPmicWrapper->Read(MT6353_TOPSTATUS, &Value);
  *Pressed = (Value & MT6353_PWRKEY_MASK) ? FALSE : TRUE;
}

VOID
HomeButtonPressed (
  OUT BOOLEAN *Pressed
  )
{
  UINT16 Value = 0;
  mPmicWrapper->Read(MT6353_TOPSTATUS, &Value);
  *Pressed = (Value & MT6353_HOMEKEY_MASK) ? FALSE : TRUE;
}

/*==================== Regulator Control ====================*/

EFI_STATUS
RegulatorSetEnable (
  IN CONST CHAR8 *Name,
  IN BOOLEAN      Enable
  )
{
  UINT16                    Value;
  CONST MTK_REGULATOR_DESC *Regulator;

  Regulator = GetRegulatorByName(Name);
  if (Regulator == NULL) {
    DEBUG((DEBUG_ERROR, "PMIC: Unknown regulator \"%a\"\n", Name));
    return EFI_INVALID_PARAMETER;
  }

  switch (Regulator->Type) {
  case Ldo:
    mPmicWrapper->Read(Regulator->Ldo.Con0Reg, &Value);
    if (Enable) {
      Value |= MT6353_CON0_ENABLE;
    } else {
      Value &= ~MT6353_CON0_ENABLE;
    }
    mPmicWrapper->Write(Regulator->Ldo.Con0Reg, Value);
    DEBUG((DEBUG_INFO, "PMIC: %a set enable=%d (CON0=0x%04x)\n", Name, Enable, Value));
    return EFI_SUCCESS;

  default:
    DEBUG((DEBUG_ERROR, "PMIC: Regulator \"%a\" type not supported in this build\n", Name));
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
RegulatorIsEnabled (
  IN  CONST CHAR8 *Name,
  OUT BOOLEAN     *Enabled
  )
{
  UINT16                    Value;
  CONST MTK_REGULATOR_DESC *Regulator;

  Regulator = GetRegulatorByName(Name);
  if (Regulator == NULL || Enabled == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Regulator->Type) {
  case Ldo:
    mPmicWrapper->Read(Regulator->Ldo.Con0Reg, &Value);
    *Enabled = (Value & MT6353_CON0_ENABLE) ? TRUE : FALSE;
    return EFI_SUCCESS;

  default:
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
RegulatorSetVoltage (
  IN CONST CHAR8 *Name,
  IN UINT32       Voltage
  )
{
  UINT16                    Value;
  CONST MTK_REGULATOR_DESC *Regulator;

  Regulator = GetRegulatorByName(Name);
  if (Regulator == NULL) {
    DEBUG((DEBUG_ERROR, "PMIC: Unknown regulator \"%a\"\n", Name));
    return EFI_INVALID_PARAMETER;
  }

  switch (Regulator->Type) {
  case Ldo: {
    UINT8  Vosel = 0;
    BOOLEAN Found = FALSE;

    // Find matching voltage and get its register encoding (Mask)
    for (UINTN Index = 0; Index < Regulator->Ldo.RangesLen; Index++) {
      if (Voltage == Regulator->Ldo.Ranges[Index].Voltage) {
        Vosel = Regulator->Ldo.Ranges[Index].Mask;   // FIX: was .Voltage before!
        Found = TRUE;
        break;
      }
    }

    if (!Found) {
      DEBUG((DEBUG_ERROR, "PMIC: \"%a\" voltage %u uV not supported\n", Name, Voltage));
      return EFI_INVALID_PARAMETER;
    }

    // Write encoded value to ANA register
    mPmicWrapper->Read(Regulator->Ldo.AnaReg, &Value);
    Value &= ~(Regulator->Ldo.VoselMask << Regulator->Ldo.VoselShift);
    Value |= ((Vosel & Regulator->Ldo.VoselMask) << Regulator->Ldo.VoselShift);
    mPmicWrapper->Write(Regulator->Ldo.AnaReg, Value);

    DEBUG((DEBUG_INFO, "PMIC: \"%a\" set voltage=%u uV (vosel=0x%x, reg=0x%04x)\n",
          Name, Voltage, Vosel, Value));
    return EFI_SUCCESS;
  }

  default:
    DEBUG((DEBUG_ERROR, "PMIC: \"%a\" set voltage not supported\n", Name));
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
RegulatorGetVoltage (
  IN  CONST CHAR8 *Name,
  OUT UINT32      *Voltage
  )
{
  UINT16                    Value;
  CONST MTK_REGULATOR_DESC *Regulator;

  Regulator = GetRegulatorByName(Name);
  if (Regulator == NULL || Voltage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Regulator->Type) {
  case Ldo:
    mPmicWrapper->Read(Regulator->Ldo.AnaReg, &Value);
    Value = (Value >> Regulator->Ldo.VoselShift) & Regulator->Ldo.VoselMask;

    for (UINTN Index = 0; Index < Regulator->Ldo.RangesLen; Index++) {
      if (Value == Regulator->Ldo.Ranges[Index].Mask) {
        *Voltage = Regulator->Ldo.Ranges[Index].Voltage;
        return EFI_SUCCESS;
      }
    }
    DEBUG((DEBUG_ERROR, "PMIC: \"%a\" unknown vosel 0x%x in ANA reg\n", Name, Value));
    return EFI_DEVICE_ERROR;

  default:
    return EFI_UNSUPPORTED;
  }
}

/*==================== Protocol Instance ====================*/

STATIC MTK_PMIC_PROTOCOL mPmic = {
  PowerButtonPressed,
  HomeButtonPressed,
  RegulatorSetEnable,
  RegulatorIsEnabled,
  RegulatorSetVoltage,
  RegulatorGetVoltage
};

/*==================== Driver Entry Point ====================*/

EFI_STATUS
EFIAPI
InitPmic (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINT16     Cid;
  UINT16     TopStatus;
  UINT16     VemcAna;

  // Locate PMIC Wrapper Protocol (communicates via WACS2)
  Status = gBS->LocateProtocol(
              &gMediaTekPmicWrapperProtocolGuid,
              NULL,
              (VOID **)&mPmicWrapper
              );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PMIC: Failed to locate wrapper protocol! %r\n", Status));
    return Status;
  }

  //
  // Debug output - verify PWRAP communication with MT6353
  //
  mPmicWrapper->Read(MT6353_CHIP_ID_REG, &Cid);
  DEBUG((DEBUG_INFO, "[PMIC] Chip ID @0x0000 = 0x%04x (expect ~0x5310 for MT6353)\n", Cid));

  mPmicWrapper->Read(MT6353_TOPSTATUS, &TopStatus);
  DEBUG((DEBUG_INFO, "[PMIC] TOPSTATUS @0x28 = 0x%04x\n", TopStatus));

  mPmicWrapper->Read(MT6353_VEMC_ANA, &VemcAna);
  DEBUG((DEBUG_INFO, "[PMIC] VEMC_ANA @0x%04x = 0x%04x\n", MT6353_VEMC_ANA, VemcAna));

  // Install MTK PMIC Protocol for MsdcImplLib to consume
  Status = gBS->InstallMultipleProtocolInterfaces(
              &ImageHandle,
              &gMediaTekPmicProtocolGuid,
              &mPmic,
              NULL
              );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PMIC: Failed to install protocol! %r\n", Status));
    return Status;
  }

  DEBUG((DEBUG_INFO, "[PMIC] MT6353 driver initialized successfully\n"));
  return EFI_SUCCESS;
}
