##
#  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
#  Copyright (c) 2018, Bingxing Wang. All rights reserved.
#  Copyright (c) Microsoft Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  #
  # Timer
  #
  USE_PHYSICAL_TIMER                         = 1

!include MediaTekPkg/MediaTekPkg.dsc.inc

[BuildOptions]
  *_*_AARCH64_CC_FLAGS = -march=armv8-a+crypto+rcpc

[PcdsFixedAtBuild]
  #
  # CPU
  #
  gArmPlatformTokenSpaceGuid.PcdCoreCount|8
  gArmPlatformTokenSpaceGuid.PcdClusterCount|2
  
  #
  # GIC (GICv2 / GIC-400)
  # mt6750.dtsi -> gic@10230000
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x10231000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x10232000
  
  #
  # gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorModel|"MediaTek (TM) MT6750 @ 1.50 GHz"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorPartNumber|"MT6750"

[LibraryClasses]
  #
  # SEC Libraries
  #
  PlatformSecLib|MT6750Pkg/Library/PlatformSecLib/PlatformSecLib.inf

  #
  # Custom Libraries
  #
  GpioImplLib|MT6750Pkg/Library/GpioImplLib/GpioImplLib.inf
  ClockImplLib|MT6750Pkg/Library/ClockImplLib/ClockImplLib.inf
  PmicWrapperImplLib|MT6750Pkg/Library/PmicWrapperImplLib/PmicWrapperImplLib.inf
  MsdcImplLib|MT6750Pkg/Library/MsdcImplLib/MsdcImplLib.inf
  
[Components]
  #
  # Custom Drivers
  # 
  MediaTekPkg/Drivers/MT6353PmicDxe/MT6353PmicDxe.inf
  