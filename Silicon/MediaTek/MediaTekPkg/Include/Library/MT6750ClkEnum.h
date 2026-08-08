#ifndef _MT6750_CLK_ENUM_H_
#define _MT6750_CLK_ENUM_H_

//
// MT6750/MT6755 Clock ID 枚举
// 名字与 MsdcImplLib.c 中的引用完全一致，不做重命名
//

typedef enum {
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
  TOP_MSDC50_0_SEL,          // ← MsdcImplLib.c 用的名字，保持不变
  TOP_MSDC30_1_SEL,          // ← 补上，MsdcImplLib.c 用的名字

  // --- INFRA Gates ---
  INFRA_APXGPT = 400,
  INFRA_UART0,
  INFRA_MSDC0,               // ← MsdcImplLib.c 用的名字，保持不变
  INFRA_MSDC1,               // ← 补上，MsdcImplLib.c 用的名字

  MAX_CLOCK_ID,              // 保持原来的结束标记
} MT6750_CLOCK_ID;

#endif // _MT6750_CLK_ENUM_H_
