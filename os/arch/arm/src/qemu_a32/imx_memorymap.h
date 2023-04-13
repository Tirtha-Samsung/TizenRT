/****************************************************************************
 * arch/arm/src/imx6/hardware/imx_memorymap.h
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

/* Reference:
 *   "i.MX 6Dual/6Quad ApplicationsProcessor Reference Manual",
 *   Document Number IMX6DQRM, Rev. 3, 07/2015, FreeScale.
 */

#ifndef __ARCH_ARM_SRC_IMX6_HARDWARE_IMX_MEMORYMAP_H
#define __ARCH_ARM_SRC_IMX6_HARDWARE_IMX_MEMORYMAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Decimal configuration values may exceed 2Gb and, hence, overflow to
 * negative values unless we force them to unsigned long:
 */

#define __CONCAT(a,b) a ## b
#define MKULONG(a) __CONCAT(a,ul)


/* i.MX6 System PSECTIONS */

#define IMX_ROMCP_PSECTION       0x00000000  /* 00000000-00017fff  96 KB Boot ROM (ROMCP) */
                                             /* 00018000-000fffff 928 KB Reserved */
#define IMX_DMA_PSECTION         0x00100000  /* 00100000-001fffff   1 MB See offsets below */
#define IMX_GPV2_PSECTION        0x00200000  /* 00200000-002fffff   1 MB GPV_2 PL301 (per1) configuration port */
#define IMX_GPV3_PSECTION        0x00300000  /* 00300000-003fffff   1 MB GPV_3 PL301 (per2) configuration port */
                                             /* 00400000-007fffff   4 MB Reserved */
#define IMX_GPV4_PSECTION        0x00800000  /* 00800000-008fffff   1 MB GPV_4 PL301 (fast3) configuration port */
#define IMX_OCRAM_PSECTION       0x00900000  /* 00900000-009fffff   1 MB OCRAM */
#define IMX_ARMMP_PSECTION       0x00a00000  /* 00a00000-00afffff   8 KB ARM MP */
#define IMX_GPV0PL301_PSECTION   0x00b00000  /* 00b00000-00bfffff   1 MB GPV0 PL301 (fast2) configuration port */
#define IMX_GPV1PL301_PSECTION   0x00c00000  /* 00c00000-00cfffff   1 MB GPV1 PL301 (fast1) configuration port */
                                             /* 00d00000-00ffffff 3072 KB Reserved */
#define IMX_PCIE_PSECTION        0x01000000  /* 01000000-01ffffff  16 MB PCIe */
#define IMX_AIPS1_PSECTION       0x02000000  /* 02000000-020fffff   1 MB Peripheral IPs via AIPS-1 */
#define IMX_AIPS2_PSECTION       0x02100000  /* 02100000-021fffff   1 MB Peripheral IPs via AIPS-2 */
#define IMX_SATA_PSECTION        0x02200000  /* 02200000-0220bfff  48 KB SATA */
                                             /* 0220c000-023fffff   2 MB Reserved */
#define IMX_IPU1_PSECTION        0x02600000  /* 02600000-029fffff   4 MB IPU-1 */
#define IMX_IPU2_PSECTION        0x02a00000  /* 02a00000-02dfffff   4 MB IPU-2 */
#define IMX_EIM_PSECTION         0x08000000  /* 08000000-0fffffff 128 MB EIM - (NOR/SRAM) */
#define IMX_MMDCDDR_PSECTION     0x10000000  /* 10000000-ffffffff 3840 MB MMDC-DDR Controller */
#  define IMX_ARMMP_VSECTION     IMX_ARMMP_PSECTION        /*   8 KB ARM MP */
                                             /* 10000000-7fffffff 1792 MB */
#define IMX_OCRAM_OFFSET         0x00000000  /* 00000000-0003ffff  0.25 MB OCRAM 256 KB */
#  define IMX_OCRAM_VSECTION     IMX_OCRAM_PSECTION        /*   1 MB OCRAM */
#define IMX_OCRAM_PBASE          (IMX_OCRAM_PSECTION+IMX_OCRAM_OFFSET)
#define IMX_OCRAM_VBASE          (IMX_OCRAM_VSECTION+IMX_OCRAM_OFFSET)
#  define IMX_OCRAM_SIZE   (256*1024)   /* Size of the On-Chip RAM (OCRAM) */
#  ifdef CONFIG_ARCH_LOWVECTORS

/* In this case, page table must lie at the top 16Kb * ncpus of OCRAM. */

#    define PGTABLE_BASE_PADDR    (IMX_OCRAM_PBASE + IMX_OCRAM_SIZE - ALL_PGTABLE_SIZE)
#    define PGTABLE_BASE_VADDR    (IMX_OCRAM_VBASE + IMX_OCRAM_SIZE - ALL_PGTABLE_SIZE)
#    define PGTABLE_IN_HIGHSRAM   1

/* We will force the IDLE stack to precede the page table */

#    define IDLE_STACK_PBASE      (PGTABLE_BASE_PADDR - CONFIG_IDLETHREAD_STACKSIZE)
#    define IDLE_STACK_VBASE      (PGTABLE_BASE_VADDR - CONFIG_IDLETHREAD_STACKSIZE)

#  else /* CONFIG_ARCH_LOWVECTORS */

/* Otherwise, the vectors lie at another location (perhaps in NOR FLASH,
 * perhaps elsewhere in OCRAM).  The page table will then be positioned
 * at the first 16Kb * ncpus of SRAM.
 */

#    define PGTABLE_BASE_PADDR    IMX_OCRAM_PBASE
#    define PGTABLE_BASE_VADDR    IMX_OCRAM_VBASE
#    define PGTABLE_IN_LOWSRAM    1

/* We will force the IDLE stack to follow the page table */

#    define IDLE_STACK_PBASE      (PGTABLE_BASE_PADDR + ALL_PGTABLE_SIZE)
#    define IDLE_STACK_VBASE      (PGTABLE_BASE_VADDR + ALL_PGTABLE_SIZE)

#  endif /* CONFIG_ARCH_LOWVECTORS */
#endif /* __ARCH_ARM_SRC_IMX6_HARDWARE_IMX_MEMORYMAP_H */
