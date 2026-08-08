#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/ClockImplLib.h>
#include <Library/MT6750ClkEnum.h>

//
// ============================================================
//  Controller Base Addresses
// ============================================================
#define AP_MIXED_BASE   0x1000c000   // apmixedsys
#define TOPCKGEN_BASE   0x10000000   // topckgen
#define INFRACFG_BASE   0x10201000   // infracfg_ao

//
// ============================================================
//  MUX 父时钟数组
// ============================================================
STATIC CONST UINT32 TopUartSelParents[] = {
  TOP_CLK26M,        // 0: 26MHz
  TOP_UNIVPLL2_D8,   // 1: UNIVPLL/3/8 ≈ 43.3MHz
};

STATIC CONST UINT32 TopAxiSelParents[] = {
  TOP_CLK26M,        // 0: 26MHz
  TOP_SYSPLL1_D4,    // 1
  TOP_SYSPLL2_D2,    // 2
  TOP_CLK26M,        // 3
};

STATIC CONST UINT32 TopMsdc50_0HclkSelParents[] = {
  TOP_CLK26M,
  TOP_SYSPLL1_D2,
  TOP_SYSPLL2_D2,
  TOP_SYSPLL2_D4,
};

STATIC CONST UINT32 TopMsdc50_0SelParents[] = {
  TOP_CLK26M,        // 0
  AP_MSDCPLL,        // 1: ~400MHz
  TOP_MSDCPLL_D2,    // 2: ~200MHz
  TOP_UNIVPLL1_D4,   // 3
  TOP_SYSPLL2_D2,    // 4
  TOP_SYSPLL_D7,     // 5
  TOP_MSDCPLL_D4,    // 6: ~100MHz
  TOP_UNIVPLL_D2,    // 7
  TOP_UNIVPLL1_D2,   // 8
};

STATIC CONST UINT32 TopMfgSelParents[] = {
  TOP_CLK26M,
  TOP_MMPLL_CK,
  TOP_UNIVPLL_D3,
  TOP_SYSPLL_D3,
};

//
// ============================================================
//  时钟描述表（严格对齐 MTK_CLOCK_DESC / MTK_PLL_DESC / MTK_GATE_DESC）
// ============================================================
MTK_CLOCK_DESC gClocks[MAX_CLOCK_ID] = {

  /* ==================== Fixed Clocks ==================== */
  [TOP_CLK26M] = {
    .Id    = TOP_CLK26M,
    .Name  = "TOP_CLK26M",
    .Type  = ClockTypeFixed,
    .Fixed = 26000000,
  },
  [TOP_F_FRTC] = {
    .Id    = TOP_F_FRTC,
    .Name  = "TOP_F_FRTC",
    .Type  = ClockTypeFixed,
    .Fixed = 32768,
  },

  /* ==================== PLLs ==================== */
  [AP_ARMSPLL] = {
    .Id         = AP_ARMSPLL,
    .Name       = "AP_ARMSPLL",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x210,
      .PowerOffset   = 0x21C,
      .EnMask        = BIT0,
      .ResetBarMask  = 0,
      .PostDivOffset = 0x214,
      .PostDivShift  = 24,
      .FMax          = 1800000000ULL,
      .FMin          = 500000000ULL,
      .PcwOffset     = 0x214,
      .PcwShift      = 0,
      .PcwBits       = 22,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },
  [AP_MAINPLL] = {
    .Id         = AP_MAINPLL,
    .Name       = "AP_MAINPLL",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x220,
      .PowerOffset   = 0x22C,
      .EnMask        = 0xF0000101,
      .ResetBarMask  = 0,
      .PostDivOffset = 0x224,
      .PostDivShift  = 24,
      .FMax          = 2400000000ULL,
      .FMin          = 800000000ULL,
      .PcwOffset     = 0x224,
      .PcwShift      = 0,
      .PcwBits       = 22,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },
  [AP_UNIVPLL] = {
    .Id         = AP_UNIVPLL,
    .Name       = "AP_UNIVPLL",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x230,
      .PowerOffset   = 0x23C,
      .EnMask        = 0xFC000001,
      .ResetBarMask  = BIT23,
      .PostDivOffset = 0x234,
      .PostDivShift  = 24,
      .FMax          = 3000000000ULL,
      .FMin          = 1200000000ULL,
      .PcwOffset     = 0x234,
      .PcwShift      = 0,
      .PcwBits       = 22,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },
  [AP_MSDCPLL] = {
    .Id         = AP_MSDCPLL,
    .Name       = "AP_MSDCPLL",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x250,
      .PowerOffset   = 0x25C,
      .EnMask        = BIT0,
      .ResetBarMask  = 0,
      .PostDivOffset = 0x254,
      .PostDivShift  = 24,
      .FMax          = 800000000ULL,
      .FMin          = 200000000ULL,
      .PcwOffset     = 0x254,
      .PcwShift      = 0,
      .PcwBits       = 22,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },
  [AP_MMPLL] = {
    .Id         = AP_MMPLL,
    .Name       = "AP_MMPLL",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x240,
      .PowerOffset   = 0x24C,
      .EnMask        = BIT0,
      .ResetBarMask  = 0,
      .PostDivOffset = 0x244,
      .PostDivShift  = 24,
      .FMax          = 1200000000ULL,
      .FMin          = 300000000ULL,
      .PcwOffset     = 0x244,
      .PcwShift      = 0,
      .PcwBits       = 22,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },
  [AP_APLL1] = {
    .Id         = AP_APLL1,
    .Name       = "AP_APLL1",
    .Controller = ClkApMixed,
    .Type       = ClockTypePll,
    .Pll        = {
      .BaseOffset    = 0x2A0,
      .PowerOffset   = 0x2B0,
      .EnMask        = BIT0,
      .ResetBarMask  = 0,
      .PostDivOffset = 0x2A4,
      .PostDivShift  = 24,
      .FMax          = 393216000ULL,
      .FMin          = 49152000ULL,
      .PcwOffset     = 0x2A8,
      .PcwShift      = 0,
      .PcwBits       = 32,
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },

  /* ==================== Factors ==================== */
  [TOP_SYSPLL_CK] = {
    .Id = TOP_SYSPLL_CK, .Name = "TOP_SYSPLL_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_SYSPLL1_CK] = {
    .Id = TOP_SYSPLL1_CK, .Name = "TOP_SYSPLL1_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL1_D2] = {
    .Id = TOP_SYSPLL1_D2, .Name = "TOP_SYSPLL1_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL1_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL1_D4] = {
    .Id = TOP_SYSPLL1_D4, .Name = "TOP_SYSPLL1_D4",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL1_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_SYSPLL2_CK] = {
    .Id = TOP_SYSPLL2_CK, .Name = "TOP_SYSPLL2_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_SYSPLL2_D2] = {
    .Id = TOP_SYSPLL2_D2, .Name = "TOP_SYSPLL2_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL2_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL2_D4] = {
    .Id = TOP_SYSPLL2_D4, .Name = "TOP_SYSPLL2_D4",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL2_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_SYSPLL_D3] = {
    .Id = TOP_SYSPLL_D3, .Name = "TOP_SYSPLL_D3",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_SYSPLL_D5] = {
    .Id = TOP_SYSPLL_D5, .Name = "TOP_SYSPLL_D5",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 5 },
  },
  [TOP_SYSPLL_D7] = {
    .Id = TOP_SYSPLL_D7, .Name = "TOP_SYSPLL_D7",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 7 },
  },
  [TOP_UNIVPLL_CK] = {
    .Id = TOP_UNIVPLL_CK, .Name = "TOP_UNIVPLL_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_UNIVPLL_D2] = {
    .Id = TOP_UNIVPLL_D2, .Name = "TOP_UNIVPLL_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL_D3] = {
    .Id = TOP_UNIVPLL_D3, .Name = "TOP_UNIVPLL_D3",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_UNIVPLL_D5] = {
    .Id = TOP_UNIVPLL_D5, .Name = "TOP_UNIVPLL_D5",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 5 },
  },
  [TOP_UNIVPLL1_CK] = {
    .Id = TOP_UNIVPLL1_CK, .Name = "TOP_UNIVPLL1_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL1_D2] = {
    .Id = TOP_UNIVPLL1_D2, .Name = "TOP_UNIVPLL1_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL1_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL1_D4] = {
    .Id = TOP_UNIVPLL1_D4, .Name = "TOP_UNIVPLL1_D4",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL1_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_UNIVPLL2_CK] = {
    .Id = TOP_UNIVPLL2_CK, .Name = "TOP_UNIVPLL2_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_UNIVPLL2_D2] = {
    .Id = TOP_UNIVPLL2_D2, .Name = "TOP_UNIVPLL2_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL2_D4] = {
    .Id = TOP_UNIVPLL2_D4, .Name = "TOP_UNIVPLL2_D4",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_UNIVPLL2_D8] = {
    .Id = TOP_UNIVPLL2_D8, .Name = "TOP_UNIVPLL2_D8",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 8 },
  },
  [TOP_MSDCPLL_CK] = {
    .Id = TOP_MSDCPLL_CK, .Name = "TOP_MSDCPLL_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_MSDCPLL_D2] = {
    .Id = TOP_MSDCPLL_D2, .Name = "TOP_MSDCPLL_D2",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_MSDCPLL_D4] = {
    .Id = TOP_MSDCPLL_D4, .Name = "TOP_MSDCPLL_D4",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 4 },
  },
  [TOP_MMPLL_CK] = {
    .Id = TOP_MMPLL_CK, .Name = "TOP_MMPLL_CK",
    .Type = ClockTypeFactors,
    .Factor = { .Parent = AP_MMPLL, .Mult = 1, .Div = 1 },
  },

  /* ==================== MUXes ==================== */
  [TOP_AXI_SEL] = {
    .Id         = TOP_AXI_SEL,
    .Name       = "TOP_AXI_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x40,
      .SetOffset    = TOPCKGEN_BASE + 0x40,
      .ClearOffset  = TOPCKGEN_BASE + 0x40,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 0,
      .MuxWidth     = 2,
      .GateShift    = 0xFF,    // 无独立 gate
      .UpdateShift  = 0,
      .Parents      = TopAxiSelParents,
      .ParentCount  = ARRAY_SIZE(TopAxiSelParents),
    },
  },
  [TOP_UART_SEL] = {
    .Id         = TOP_UART_SEL,
    .Name       = "TOP_UART_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x60,
      .SetOffset    = TOPCKGEN_BASE + 0x60,
      .ClearOffset  = TOPCKGEN_BASE + 0x60,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 8,
      .MuxWidth     = 1,
      .GateShift    = 15,
      .UpdateShift  = 9,
      .Parents      = TopUartSelParents,
      .ParentCount  = ARRAY_SIZE(TopUartSelParents),
    },
  },
  [TOP_MFG_SEL] = {
    .Id         = TOP_MFG_SEL,
    .Name       = "TOP_MFG_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x50,
      .SetOffset    = TOPCKGEN_BASE + 0x50,
      .ClearOffset  = TOPCKGEN_BASE + 0x50,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 24,
      .MuxWidth     = 2,
      .GateShift    = 31,
      .UpdateShift  = 7,
      .Parents      = TopMfgSelParents,
      .ParentCount  = ARRAY_SIZE(TopMfgSelParents),
    },
  },
  [TOP_MSDC50_0_HCLK_SEL] = {
    .Id         = TOP_MSDC50_0_HCLK_SEL,
    .Name       = "TOP_MSDC50_0_HCLK_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x70,
      .SetOffset    = TOPCKGEN_BASE + 0x70,
      .ClearOffset  = TOPCKGEN_BASE + 0x70,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 8,
      .MuxWidth     = 2,
      .GateShift    = 15,
      .UpdateShift  = 12,
      .Parents      = TopMsdc50_0HclkSelParents,
      .ParentCount  = ARRAY_SIZE(TopMsdc50_0HclkSelParents),
    },
  },
  [TOP_MSDC50_0_SEL] = {
    .Id         = TOP_MSDC50_0_SEL,
    .Name       = "TOP_MSDC50_0_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x70,
      .SetOffset    = TOPCKGEN_BASE + 0x70,
      .ClearOffset  = TOPCKGEN_BASE + 0x70,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 16,
      .MuxWidth     = 4,
      .GateShift    = 23,
      .UpdateShift  = 13,
      .Parents      = TopMsdc50_0SelParents,
      .ParentCount  = ARRAY_SIZE(TopMsdc50_0SelParents),
    },
  },
  [TOP_MSDC30_1_SEL] = {
    .Id         = TOP_MSDC30_1_SEL,
    .Name       = "TOP_MSDC30_1_SEL",
    .Controller = ClkTopCkGen,
    .Type       = ClockTypeMuxGate,
    .MuxGate    = {
      .MuxOffset    = TOPCKGEN_BASE + 0x70,   // CLK_CFG_3
      .SetOffset    = TOPCKGEN_BASE + 0x70,
      .ClearOffset  = TOPCKGEN_BASE + 0x70,
      .UpdateOffset = TOPCKGEN_BASE + 0x04,
      .MuxShift     = 8,                       // [10:8]
      .MuxWidth     = 3,
      .GateShift    = 15,
      .UpdateShift  = 9,
      .Parents      = TopMsdc50_0SelParents,   // 复用同一组父时钟
      .ParentCount  = ARRAY_SIZE(TopMsdc50_0SelParents),
    },
  },

  /* ==================== INFRA Gates ==================== */
  [INFRA_APXGPT] = {
    .Id         = INFRA_APXGPT,
    .Name       = "INFRA_APXGPT",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .SetOffset   = INFRACFG_BASE + 0x80,   // infra0 SET
      .ClearOffset = INFRACFG_BASE + 0x84,   // infra0 CLR
      .StatusOffset = 0,
      .GateShift   = 6,
      .Inverted    = FALSE,
      .Parent      = 0,
    },
  },
  [INFRA_UART0] = {
    .Id         = INFRA_UART0,
    .Name       = "INFRA_UART0",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .SetOffset   = INFRACFG_BASE + 0x80,
      .ClearOffset = INFRACFG_BASE + 0x84,
      .StatusOffset = 0,
      .GateShift   = 22,
      .Inverted    = FALSE,
      .Parent      = 0,
    },
  },
  [INFRA_MSDC0] = {
    .Id         = INFRA_MSDC0,
    .Name       = "INFRA_MSDC0",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .SetOffset   = INFRACFG_BASE + 0x88,   // infra1 SET
      .ClearOffset = INFRACFG_BASE + 0x8C,   // infra1 CLR
      .StatusOffset = 0,
      .GateShift   = 2,
      .Inverted    = FALSE,
      .Parent      = 0,
    },
  },
  [INFRA_MSDC1] = {
    .Id         = INFRA_MSDC1,
    .Name       = "INFRA_MSDC1",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .SetOffset   = INFRACFG_BASE + 0x88,   // infra1 SET
      .ClearOffset = INFRACFG_BASE + 0x8C,   // infra1 CLR
      .StatusOffset = 0,
      .GateShift   = 4,
      .Inverted    = FALSE,
      .Parent      = 0,
    },
  },
};

UINTN gClockCount = MAX_CLOCK_ID;

//
// ============================================================
//  Helper
// ============================================================
STATIC UINTN GetControllerBase(UINT8 Controller)
{
  switch (Controller) {
  case ClkApMixed:  return AP_MIXED_BASE;
  case ClkTopCkGen: return TOPCKGEN_BASE;
  case ClkInfraCfg: return INFRACFG_BASE;
  default:          return 0;
  }
}

//
// ============================================================
//  Enable
// ============================================================
EFI_STATUS EFIAPI ClockImplEnable(IN UINT32 ClockId)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;

  switch (Desc->Type) {
  case ClockTypeGate:
    MmioOr32(Desc->Gate.SetOffset, BIT(Desc->Gate.GateShift));
    break;

  case ClockTypeMuxGate:
    if (Desc->MuxGate.GateShift != 0xFF) {
      // 开启 gate：清 0
      MmioAnd32(Desc->MuxGate.MuxOffset,
                ~(BIT(Desc->MuxGate.GateShift)));
    }
    break;

  case ClockTypePll: {
    UINTN Base = GetControllerBase(Desc->Controller);
    // PWR_ON = 写 0
    MmioWrite32(Base + Desc->Pll.PowerOffset, 0x00000000);
    // 使能 PLL
    MmioOr32(Base + Desc->Pll.BaseOffset, Desc->Pll.EnMask);
    break;
  }

  default:
    break;
  }

  return EFI_SUCCESS;
}

//
// ============================================================
//  Disable
// ============================================================
EFI_STATUS EFIAPI ClockImplDisable(IN UINT32 ClockId)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;

  switch (Desc->Type) {
  case ClockTypeGate:
    MmioOr32(Desc->Gate.ClearOffset, BIT(Desc->Gate.GateShift));
    break;

  case ClockTypeMuxGate:
    if (Desc->MuxGate.GateShift != 0xFF) {
      MmioOr32(Desc->MuxGate.MuxOffset, BIT(Desc->MuxGate.GateShift));
    }
    break;

  case ClockTypePll: {
    UINTN Base = GetControllerBase(Desc->Controller);
    // PWR_OFF = 写 1
    MmioWrite32(Base + Desc->Pll.PowerOffset, 0x00000001);
    break;
  }

  default:
    break;
  }

  return EFI_SUCCESS;
}

//
// ============================================================
//  GetRate
// ============================================================
UINT64 EFIAPI ClockImplGetRate(IN UINT32 ClockId)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return 0;

  switch (Desc->Type) {
  case ClockTypeFixed:
    return Desc->Fixed;

  case ClockTypeFactors:
    return ClockImplGetRate(Desc->Factor.Parent) *
           Desc->Factor.Mult / Desc->Factor.Div;

  case ClockTypePll:
    return Desc->Pll.FMin;

  case ClockTypeMuxGate: {
    UINTN Base = GetControllerBase(Desc->Controller);
    UINT32 Val = MmioRead32(Desc->MuxGate.MuxOffset);
    UINT32 Sel = (Val >> Desc->MuxGate.MuxShift) &
                 ((1 << Desc->MuxGate.MuxWidth) - 1);
    if (Sel < Desc->MuxGate.ParentCount)
      return ClockImplGetRate(Desc->MuxGate.Parents[Sel]);
    return 0;
  }

  default:
    return 0;
  }
}

//
// ============================================================
//  SetRate (UEFI 阶段不需要动态调频)
// ============================================================
EFI_STATUS EFIAPI ClockImplSetRate(IN UINT32 ClockId, IN UINT64 Rate)
{
  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;
  return EFI_SUCCESS;
}

//
// ============================================================
//  Init
// ============================================================
VOID EFIAPI ClockImplInit(VOID)
{
  DEBUG((DEBUG_INFO, "MT6750: ClockImplInit start\n"));

  ClockImplEnable(AP_ARMSPLL);
  ClockImplEnable(AP_MAINPLL);
  ClockImplEnable(AP_UNIVPLL);
  ClockImplEnable(AP_MSDCPLL);
  ClockImplEnable(AP_MMPLL);
  ClockImplEnable(AP_APLL1);

  // 等待 PLL 锁定
  MicroSecondDelay(200);

  ClockImplEnable(INFRA_APXGPT);
  ClockImplEnable(INFRA_UART0);

  DEBUG((DEBUG_INFO, "MT6750: ClockImplInit done\n"));
}
