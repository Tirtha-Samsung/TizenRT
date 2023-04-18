/****************************************************************************
 * arch/arm64/include/qemu/chip.h
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

#ifndef __ARCH_ARM32_INCLUDE_QEMU_CHIP_H
#define __ARCH_ARM32_INCLUDE_QEMU_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#ifndef __ASSEMBLY__
#  include <tinyara/arch.h>
#endif
//#include "a1x_memorymap.h"
/* Number of bytes in @p x kibibytes/mebibytes/gibibytes */

#define KB(x)           ((x) << 10)
#define MB(x)           (KB(x) << 10)
#define GB(x)           (MB(UINT64_C(x)) << 10)

#if defined(CONFIG_ARCH_CHIP_QEMU_A32)

#define CONFIG_GICD_BASE          0x8000000
#define CONFIG_GICR_BASE          0x80a0000

#define CONFIG_RAMBANK1_ADDR      0x40000000
#define CONFIG_RAMBANK1_SIZE      MB(4)

#define CONFIG_DEVICEIO_BASEADDR  0x7000000
#define CONFIG_DEVICEIO_SIZE      MB(512)

#define CONFIG_LOAD_BASE          0x40280000
//#define CONFIG_LOAD_BASE          0x4a000000

#endif

#define CHIP_MPCORE_VBASE (CONFIG_RAMBANK1_ADDR + (10 * 1024 * 1024))


#define TIZENRT_TEXT_VADDR     _stext_ram
#define TIZENRT_TEXT_PADDR     _stext_ram
#define TIZENRT_TEXT_PEND      _sdata
#define TIZENRT_TEXT_SIZE      0xfff00000 //(TIZENRT_TEXT_PEND - TIZENRT_TEXT_PADDR)
//#define TIZENRT_TEXT_SIZE      (TIZENRT_TEXT_PEND - TIZENRT_TEXT_PADDR)


#  define TIZENRT_RAM_VADDR        _sdata
#  define TIZENRT_RAM_PADDR        _sdata
#  define TIZENRT_RAM_PEND         ((CONFIG_RAM_END + 0x000fffff) & 0xfff00000)
//#  define TIZENRT_RAM_SIZE      (TIZENRT_RAM_PEND - TIZENRT_RAM_PADDR)
#  define TIZENRT_RAM_SIZE	10

#    define PGTABLE_BASE_PADDR  CONFIG_RAMBANK1_ADDR
#    define PGTABLE_BASE_VADDR  CONFIG_RAMBANK1_ADDR

#define VECTOR_TABLE_SIZE         0x00010000
#define VECTOR_TABLE_OFFSET       0x00000040

#  define QEMU_VECTOR_PADDR        _stext_ram
#  define QEMU_VECTOR_VSRAM        _stext_ram

#ifdef CONFIG_ARCH_LOWVECTORS  /* Vectors located at 0x0000:0000  */

#  define QEMU_VECTOR_VADDR        0x00000000

#else  /* Vectors located at 0xffff:0000 -- this probably does not work */

#  define QEMU_VECTOR_VADDR        0xffff0000

#endif

#ifndef CONFIG_ARCH_LOWVECTORS
/* Vector L2 page table offset/size */

#  define VECTOR_L2_OFFSET        0x000000400
#  define VECTOR_L2_SIZE          0x000000bfc

/* Vector L2 page table base addresses */

#  define VECTOR_L2_PBASE         (PGTABLE_BASE_PADDR+VECTOR_L2_OFFSET)
#  define VECTOR_L2_VBASE         (PGTABLE_BASE_VADDR+VECTOR_L2_OFFSET)

/* Vector L2 page table end addresses */

#  define VECTOR_L2_END_PADDR     (VECTOR_L2_PBASE+VECTOR_L2_SIZE)
#  define VECTOR_L2_END_VADDR     (VECTOR_L2_VBASE+VECTOR_L2_SIZE)

#endif /* !CONFIG_ARCH_LOWVECTORS */



#define ARMV7A_PGTABLE_MAPPING 1
#endif /* __ARCH_ARM32_INCLUDE_QEMU_CHIP_H */
