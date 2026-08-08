##
#  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
#  Copyright (c) 2018, Bingxing Wang. All rights reserved.
#  Copyright (c) Microsoft Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = a37
  PLATFORM_GUID                  = 0BDC7D8A-C915-4CDA-BB1D-78D7E1C1FD1E
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/a37Pkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = RELEASE|DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = a37Pkg/a37.fdf
  USE_CUSTOM_DISPLAY_DRIVER      = 0

!include MT6750Pkg/MT6750Pkg.dsc.inc

[PcdsFixedAtBuild]
  #
  # DDR Memory
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  #
  # UEFI Stack
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x40200000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Oppo"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"OPPO A37"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"a37"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"OPPO_A37_a37"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"OPPO A37"

  #
  # Simple Frame Buffer
  #
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|720
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|1280
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32

[LibraryClasses]
  #
  # Memory Libraries
  #
  MemoryMapLib|a37Pkg/Library/MemoryMapLib/MemoryMapLib.inf

  #
  # Input Libraries
  #
  KeypadDeviceLib|a37Pkg/Library/KeypadDeviceLib/KeypadDeviceLib.inf

[Components]
  #
  # Input
  #
  SiliciumPkg/Drivers/KeypadDxe/KeypadDxe.inf
  SiliciumPkg/Drivers/KeypadDeviceDxe/KeypadDeviceDxe.inf
