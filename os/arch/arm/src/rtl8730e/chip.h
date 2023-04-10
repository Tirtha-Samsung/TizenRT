/****************************************************************************
 * arch/arm/src/rtl8730e/chip.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RTL8730E_CHIP_H
#define __ARCH_ARM_SRC_RTL8730E_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

//#include "hardware/rtl_memorymap.h"

#define RTL8730E_SRAMA2_OFFSET    0x00004000
#define RTL8730E_INTMEM_VSECTION  0x00000000
#define RTL8730E_SRAMA2_VADDR     (RTL8730E_INTMEM_VSECTION+RTL8730E_SRAMA2_OFFSET)
#define PGTABLE_BASE_VADDR  RTL8730E_SRAMA2_VADDR
#define RTL8730E_INTMEM_PSECTION  0x00000000
#define RTL8730E_SRAMA2_PADDR     (RTL8730E_INTMEM_PSECTION+RTL8730E_SRAMA2_OFFSET)
#define PGTABLE_BASE_PADDR  RTL8730E_SRAMA2_PADDR
#define NUTTX_TEXT_VADDR     (CONFIG_RAM_VSTART & 0xfff00000)
#define NUTTX_TEXT_PADDR     (CONFIG_RAM_START & 0xfff00000)
#define NUTTX_TEXT_PEND      ((CONFIG_RAM_END + 0x000fffff) & 0xfff00000)
#define NUTTX_TEXT_SIZE      (NUTTX_TEXT_PEND - NUTTX_TEXT_PADDR)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

#endif 
