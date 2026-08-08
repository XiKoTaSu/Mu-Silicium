/** @file
  MT6750 Clock Implementation Library
  Based on MT6755 clock driver (clk-mt6755.c)

  Copyright (c) 2025, Your Name. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/*#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>*/
#include <Library/ClockImplLib.h>
#include <MT6750ClkEnum.h>

//
// ============================================================
//  Controller Base Addresses
// ============================================================
#define AP_MIXED_BASE   0x1000c000   // apmixedsys
#define TOPCKGEN_BASE   0x10000000   // topckgen
#define INFRACFG_BASE   0x10201000   // infracfg

#ifndef ClkApMixed
#define ClkApMixed      0
#define ClkTopCkGen     1
#define ClkInfraCfg     2
#endif

//
// ============================================================
//  Clock ID 枚举
// ============================================================
/*typedef enum {
  // --- Fixed ---
  TOP_CLK26M = 0,
  TOP_F_FRTC,

  // --- PLLs (APMIXED) ---
  AP_ARMSPLL = 100,
  AP_MAINPLL,
  AP_UNIVPLL,
  AP_MSDCPLL,
  AP_MMPLL,
  AP_APLL1,

  // --- Factors ---
  TOP_SYSPLL_CK = 200,
  TOP_SYSPLL1_CK,
  TOP_SYSPLL1_D2,
  TOP_SYSPLL1_D4,
  TOP_SYSPLL2_CK,
  TOP_SYSPLL2_D2,
  TOP_SYSPLL2_D4,
  TOP_SYSPLL_D3,
  TOP_SYSPLL_D5,
  TOP_SYSPLL_D7,
  TOP_UNIVPLL_CK,
  TOP_UNIVPLL_D2,
  TOP_UNIVPLL_D3,
  TOP_UNIVPLL_D5,
  TOP_UNIVPLL1_CK,
  TOP_UNIVPLL1_D2,
  TOP_UNIVPLL1_D4,
  TOP_UNIVPLL2_CK,
  TOP_UNIVPLL2_D2,
  TOP_UNIVPLL2_D4,
  TOP_UNIVPLL2_D8,
  TOP_MSDCPLL_CK,
  TOP_MSDCPLL_D2,
  TOP_MSDCPLL_D4,
  TOP_MMPLL_CK,

  // --- MUXes (TOPCKGEN) ---
  TOP_AXI_SEL = 300,
  TOP_UART_SEL,
  TOP_MFG_SEL,
  TOP_MSDC50_0_HCLK_SEL,
  TOP_MSDC50_0_SEL,

  // --- INFRA Gates ---
  INFRA_APXGPT = 400,
  INFRA_UART0,
  INFRA_MSDC0,

  MAX_CLOCK_ID,
} MT6750_CLOCK_ID;*/

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
  TOP_SYSPLL1_D4,    // 1: MAINPLL/2/4
  TOP_SYSPLL2_D2,    // 2: MAINPLL/3/2
  TOP_CLK26M,        // 3: placeholder (osc_d8 not needed in UEFI)
};

STATIC CONST UINT32 TopMsdc50_0HclkSelParents[] = {
  TOP_CLK26M,        // 0
  TOP_SYSPLL1_D2,    // 1: MAINPLL/2
  TOP_SYSPLL2_D2,    // 2: MAINPLL/3/2
  TOP_SYSPLL2_D4,    // 3: MAINPLL/3/4 (approximation for SYSPLL4_D2)
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
  TOP_CLK26M,        // 0
  TOP_MMPLL_CK,      // 1: MMPLL
  TOP_UNIVPLL_D3,    // 2
  TOP_SYSPLL_D3,     // 3
};

//
// ============================================================
//  时钟描述表
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
      .PowerOnVal    = 0x00000000,  // 写0上电（MTK PWR_CON惯例）
      .PowerOffVal   = 0x00000001,  // 写1掉电
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
      .PowerOnVal    = 0x00000000,
      .PowerOffVal   = 0x00000001,
      .EnMask        = 0xF0000101,  // Linux 原始值
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
      .PowerOnVal    = 0x00000000,
      .PowerOffVal   = 0x00000001,
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
      .PowerOnVal    = 0x00000000,
      .PowerOffVal   = 0x00000001,
      .EnMask        = BIT0,
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
      .PowerOnVal    = 0x00000000,
      .PowerOffVal   = 0x00000001,
      .EnMask        = BIT0,
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
      .PowerOnVal    = 0x00000000,
      .PowerOffVal   = 0x00000001,
      .EnMask        = BIT0,
      .PostDivOffset = 0x2A4,
      .PostDivShift  = 24,
      .FMax          = 393216000ULL,  // 音频常用 49.152MHz × 8
      .FMin          = 49152000ULL,
      .PcwOffset     = 0x2A8,        // APLL 的 PCW 在 CON2
      .PcwShift      = 0,
      .PcwBits       = 32,           // ★ APLL 是 32 位
      .PcwiBits      = 8,
      .Parent        = TOP_CLK26M,
    },
  },

  /* ==================== Factors ==================== */
  [TOP_SYSPLL_CK] = {
    .Id     = TOP_SYSPLL_CK, .Name = "TOP_SYSPLL_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_SYSPLL1_CK] = {
    .Id     = TOP_SYSPLL1_CK, .Name = "TOP_SYSPLL1_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL1_D2] = {
    .Id     = TOP_SYSPLL1_D2, .Name = "TOP_SYSPLL1_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL1_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL1_D4] = {
    .Id     = TOP_SYSPLL1_D4, .Name = "TOP_SYSPLL1_D4",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL1_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_SYSPLL2_CK] = {
    .Id     = TOP_SYSPLL2_CK, .Name = "TOP_SYSPLL2_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_SYSPLL2_D2] = {
    .Id     = TOP_SYSPLL2_D2, .Name = "TOP_SYSPLL2_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL2_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_SYSPLL2_D4] = {
    .Id     = TOP_SYSPLL2_D4, .Name = "TOP_SYSPLL2_D4",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_SYSPLL2_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_SYSPLL_D3] = {
    .Id     = TOP_SYSPLL_D3, .Name = "TOP_SYSPLL_D3",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_SYSPLL_D5] = {
    .Id     = TOP_SYSPLL_D5, .Name = "TOP_SYSPLL_D5",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 5 },
  },
  [TOP_SYSPLL_D7] = {
    .Id     = TOP_SYSPLL_D7, .Name = "TOP_SYSPLL_D7",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MAINPLL, .Mult = 1, .Div = 7 },
  },
  [TOP_UNIVPLL_CK] = {
    .Id     = TOP_UNIVPLL_CK, .Name = "TOP_UNIVPLL_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_UNIVPLL_D2] = {
    .Id     = TOP_UNIVPLL_D2, .Name = "TOP_UNIVPLL_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL_D3] = {
    .Id     = TOP_UNIVPLL_D3, .Name = "TOP_UNIVPLL_D3",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_UNIVPLL_D5] = {
    .Id     = TOP_UNIVPLL_D5, .Name = "TOP_UNIVPLL_D5",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 5 },
  },
  [TOP_UNIVPLL1_CK] = {
    .Id     = TOP_UNIVPLL1_CK, .Name = "TOP_UNIVPLL1_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL1_D2] = {
    .Id     = TOP_UNIVPLL1_D2, .Name = "TOP_UNIVPLL1_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL1_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL1_D4] = {
    .Id     = TOP_UNIVPLL1_D4, .Name = "TOP_UNIVPLL1_D4",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL1_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_UNIVPLL2_CK] = {
    .Id     = TOP_UNIVPLL2_CK, .Name = "TOP_UNIVPLL2_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_UNIVPLL, .Mult = 1, .Div = 3 },
  },
  [TOP_UNIVPLL2_D2] = {
    .Id     = TOP_UNIVPLL2_D2, .Name = "TOP_UNIVPLL2_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 2 },
  },
  [TOP_UNIVPLL2_D4] = {
    .Id     = TOP_UNIVPLL2_D4, .Name = "TOP_UNIVPLL2_D4",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 4 },
  },
  [TOP_UNIVPLL2_D8] = {
    .Id     = TOP_UNIVPLL2_D8, .Name = "TOP_UNIVPLL2_D8",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = TOP_UNIVPLL2_CK, .Mult = 1, .Div = 8 },
  },
  [TOP_MSDCPLL_CK] = {
    .Id     = TOP_MSDCPLL_CK, .Name = "TOP_MSDCPLL_CK",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 1 },
  },
  [TOP_MSDCPLL_D2] = {
    .Id     = TOP_MSDCPLL_D2, .Name = "TOP_MSDCPLL_D2",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 2 },
  },
  [TOP_MSDCPLL_D4] = {
    .Id     = TOP_MSDCPLL_D4, .Name = "TOP_MSDCPLL_D4",
    .Type   = ClockTypeFactors,
    .Factor = { .Parent = AP_MSDCPLL, .Mult = 1, .Div = 4 },
  },
  [TOP_MMPLL_CK] = {
    .Id     = TOP_MMPLL_CK, .Name = "TOP_MMPLL_CK",
    .Type   = ClockTypeFactors,
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
      .GateShift    = 0xFFFFFFFF,  // 无独立 gate
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
      .GateShift     = 15,
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
      .RegOffset = INFRACFG_BASE + 0x80,   // infra0 SET
      .Bit       = 6,
    },
  },
  [INFRA_UART0] = {
    .Id         = INFRA_UART0,
    .Name       = "INFRA_UART0",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .RegOffset = INFRACFG_BASE + 0x80,   // infra0 SET
      .Bit       = 22,
    },
  },
  [INFRA_MSDC0] = {
    .Id         = INFRA_MSDC0,
    .Name       = "INFRA_MSDC0",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .RegOffset = INFRACFG_BASE + 0x88,   // infra1 SET
      .Bit       = 2,
    },
  },
  [INFRA_MSDC1] = {
    .Id         = INFRA_MSDC1,
    .Name       = "INFRA_MSDC1",
    .Controller = ClkInfraCfg,
    .Type       = ClockTypeGate,
    .Gate       = {
      .RegOffset = INFRACFG_BASE + 0x88,       // infra1 SET
      .Bit       = 4,                           // bit 4 = MSDC1
    },
  },
};

UINTN gClockCount = MAX_CLOCK_ID;

//
// ============================================================
//  Library Functions
// ============================================================

/**
  获取控制器基地址（如果你的框架已经有这个函数，删掉这段）
**/
STATIC UINTN GetControllerBase(UINT8 Controller)
{
  switch (Controller) {
  case ClkApMixed:  return AP_MIXED_BASE;
  case ClkTopCkGen: return TOPCKGEN_BASE;
  case ClkInfraCfg: return INFRACFG_BASE;
  default:          return 0;
  }
}

/**
  启用时钟 — 对 Gate 类型写 SET 寄存器置位
**/
EFI_STATUS EFIAPI ClockImplEnable(IN UINT32 ClockId)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;

  switch (Desc->Type) {
  case ClockTypeGate:
    // 写 SET 寄存器，对应的 bit 置 1 = 开启时钟
    MmioOr32(Desc->Gate.RegOffset, BIT(Desc->Gate.Bit));
    break;

  case ClockTypeMuxGate:
    if (Desc->MuxGate.GateShift != 0xFFFFFFFF) {
      // 有独立 gate bit，关闭 gate（写 0 到 MuxOffset 对应位）
      // MT6755: gate bit 在 MuxOffset 里，写 0 开启
      MmioAnd32(Desc->MuxGate.MuxOffset,
                ~(BIT(Desc->MuxGate.GateShift)));
    }
    break;

  case ClockTypePll: {
    // 上电 + 使能
    UINTN Base = GetControllerBase(Desc->Controller);
    MmioWrite32(Base + Desc->Pll.PowerOffset, Desc->Pll.PowerOnVal);
    MmioOr32(Base + Desc->Pll.BaseOffset, Desc->Pll.EnMask);
    break;
  }

  default:
    // Fixed / Factors 不需要 enable
    break;
  }

  return EFI_SUCCESS;
}

/**
  禁用时钟
**/
EFI_STATUS EFIAPI ClockImplDisable(IN UINT32 ClockId)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;

  switch (Desc->Type) {
  case ClockTypeGate:
    // 写 CLR 寄存器（SET 偏移 +4）
    MmioOr32(Desc->Gate.RegOffset + 0x04, BIT(Desc->Gate.Bit));
    break;

  case ClockTypeMuxGate:
    if (Desc->MuxGate.GateShift != 0xFFFFFFFF) {
      MmioOr32(Desc->MuxGate.MuxOffset, BIT(Desc->MuxGate.GateShift));
    }
    break;

  case ClockTypePll: {
    UINTN Base = GetControllerBase(Desc->Controller);
    MmioWrite32(Base + Desc->Pll.PowerOffset, Desc->Pll.PowerOffVal);
    break;
  }

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  获取时钟频率（递归向上遍历时钟树）
**/
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

  case ClockTypePll: {
    // 简化：返回 FMin（UEFI 阶段不精确算频率也没事）
    // 完整实现需要读 PCW 寄存器算
    return Desc->Pll.FMin;
  }

  case ClockTypeMuxGate: {
    UINTN Base = GetControllerBase(Desc->Controller);
    UINT32 Val = MmioRead32(Desc->MuxGate.MuxOffset);
    UINT32 Sel = (Val >> Desc->MuxGate.MuxShift) &
                 ((1 << Desc->MuxGate.MuxWidth) - 1);
    if (Sel < Desc->MuxGate.ParentCount)
      return ClockImplGetRate(Desc->MuxGate.Parents[Sel]);
    return 0;
  }

  case ClockTypeGate:
    // Gate 不改变频率，返回父时钟的频率
    // 简化：返回 0（INFRA gate 的父频率由 MUX 决定，UEFI 不查）
    return 0;

  default:
    return 0;
  }
}

/**
  设置时钟频率（简化版：PLL 不支持动态调频，MUX 选合适父时钟）
**/
EFI_STATUS EFIAPI ClockImplSetRate(IN UINT32 ClockId, IN UINT64 Rate)
{
  MTK_CLOCK_DESC *Desc = &gClocks[ClockId];

  if (ClockId >= MAX_CLOCK_ID)
    return EFI_INVALID_PARAMETER;

  // UEFI 阶段一般不需要动态设频率
  // 如果需要，在这里添加 PLL PCW 计算和 MUX 父选择逻辑
  return EFI_SUCCESS;
}

/**
  初始化所有 PLL（在 SEC/PEI 阶段早期调用）
**/
VOID EFIAPI ClockImplInit(VOID)
{
  DEBUG((DEBUG_INFO, "MT6750: ClockImplInit start\n"));

  // 上电并使能所有 PLL
  ClockImplEnable(AP_ARMSPLL);
  ClockImplEnable(AP_MAINPLL);
  ClockImplEnable(AP_UNIVPLL);
  ClockImplEnable(AP_MSDCPLL);
  ClockImplEnable(AP_MMPLL);
  ClockImplEnable(AP_APLL1);

  // 等待 PLL 锁定（简化：微秒延迟）
  // 实际应该轮询 CON1 的锁定位
  MicroSecondDelay(200);

  // 使能 INFRA 关键门控
  ClockImplEnable(INFRA_APXGPT);
  ClockImplEnable(INFRA_UART0);

  DEBUG((DEBUG_INFO, "MT6750: ClockImplInit done\n"));
}

UINTN gClockCount = ARRAY_SIZE (gClocks);