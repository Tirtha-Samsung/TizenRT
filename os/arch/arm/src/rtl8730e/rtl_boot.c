/****************************************************************************
 * arch/arm/src/rtl8730e/rtl_boot.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <assert.h>

#ifdef CONFIG_PAGING
#  include <tinyara/page.h>
#endif

#include "chip.h"
#include "arm.h"
#include "mmu.h"
#include "up_internal.h"
#include "rtl_boot.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The vectors are, by default, positioned at the beginning of the text
 * section.  They will always have to be copied to the correct location.
 *
 * If we are using high vectors (CONFIG_ARCH_LOWVECTORS=n).  In this case,
 * the vectors will lie at virtual address 0xffff:000 and we will need
 * to a) copy the vectors to another location, and b) map the vectors
 * to that address, and
 *
 * For the case of CONFIG_ARCH_LOWVECTORS=y, defined.  Vectors will be
 * copied to SRAM A1 at address 0x0000:0000
 */

#if !defined(CONFIG_ARCH_LOWVECTORS) && defined(CONFIG_ARCH_ROMPGTABLE)
#  error High vector remap cannot be performed if we are using a ROM page table
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern uint8_t _vector_start[]; /* Beginning of vector block */
extern uint8_t _vector_end[];   /* End+1 of vector block */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This table describes how to map a set of 1Mb pages to space the physical
 * address space of the RTL8730E.
 */

#ifndef CONFIG_ARCH_ROMPGTABLE
static const struct section_mapping_s section_mapping[] =
{
#if 0
  { RTL8730E_INTMEM_PSECTION,  RTL8730E_INTMEM_VSECTION,  /* Includes vectors and page table */
    RTL8730E_INTMEM_MMUFLAGS,  RTL8730E_INTMEM_NSECTIONS
  },
  { RTL8730E_PERIPH_PSECTION,  RTL8730E_PERIPH_VSECTION,
    RTL8730E_PERIPH_MMUFLAGS,  RTL8730E_PERIPH_NSECTIONS
  },
  { RTL8730E_SRAMC_PSECTION,   RTL8730E_SRAMC_VSECTION,
    RTL8730E_SRAMC_MMUFLAGS,   RTL8730E_SRAMC_NSECTIONS
  },
  { RTL8730E_DE_PSECTION,      RTL8730E_DE_VSECTION,
    RTL8730E_DE_MMUFLAGS,      RTL8730E_DE_NSECTIONS
  },
  { RTL8730E_DDR_MAPPADDR,     RTL8730E_DDR_MAPVADDR,
    RTL8730E_DDR_MMUFLAGS,     RTL8730E_DDR_NSECTIONS
  },
  { RTL8730E_BROM_PSECTION,    RTL8730E_BROM_VSECTION,
    RTL8730E_BROM_MMUFLAGS,    RTL8730E_BROM_NSECTIONS
  }
#endif
};

#define NMAPPINGS \
  (sizeof(section_mapping) / sizeof(struct section_mapping_s))
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rtl_setupmappings
 *
 * Description:
 *   Map all of the initial memory regions defined in section_mapping[]
 *
 ****************************************************************************/

#ifndef CONFIG_ARCH_ROMPGTABLE
static inline void rtl_setupmappings(void)
{
  mmu_l1_map_regions(section_mapping, NMAPPINGS);
}
#endif

/****************************************************************************
 * Name: rtl_vectorpermissions
 *
 * Description:
 *   Set permissions on the vector mapping.
 *
 ****************************************************************************/

#if !defined(CONFIG_ARCH_ROMPGTABLE) && defined(CONFIG_ARCH_LOWVECTORS) && \
     defined(CONFIG_PAGING)
static void rtl_vectorpermissions(uint32_t mmuflags)
{
  /* The PTE for the beginning of ISRAM is at the base of the L2 page table */

  uint32_t pte = mmu_l2_getentry(PG_L2_VECT_VADDR, 0);

  /* Mask out the old MMU flags from the page table entry.
   *
   * The pte might be zero the first time this function is called.
   */

  if (pte == 0)
    {
      pte = PG_VECT_PBASE;
    }
  else
    {
      pte &= PG_L1_PADDRMASK;
    }

  /* Update the page table entry with the MMU flags and save */

  mmu_l2_setentry(PG_L2_VECT_VADDR, pte, 0, mmuflags);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_boot
 *
 * Description:
 *   Complete boot operations started in arm_head.S
 *
 *   This logic will be executing in SDRAM.  This boot logic was started by
 *   the A10 boot logic.  At this point in time, clocking and SDRAM have
 *   already be initialized (they must be because we are executing out of
 *   SDRAM).  So all that must be done here is to:
 *
 *     1) Refine the memory mapping,
 *     2) Configure the serial console, and
 *     3) Perform board-specific initializations.
 *
 ****************************************************************************/

void arm_boot(void)
{
#ifndef CONFIG_ARCH_ROMPGTABLE
  /* __start provided the basic MMU mappings for SRAM.  Now provide mappings
   * for all IO regions (Including the vector region).
   */

  rtl_setupmappings();

#endif
}

