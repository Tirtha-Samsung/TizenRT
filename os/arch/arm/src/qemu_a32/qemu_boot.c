/****************************************************************************
 * arch/arm64/src/qemu/qemu_boot.c
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
#include <debug.h>

#ifdef CONFIG_PAGING
#  include <tinyara/page.h>
#endif

#include "chip.h"

#ifdef CONFIG_SMP
#include "smp.h"
#endif

#include <tinyara/arch.h>
#include "up_internal.h"
#include "mmu.h"
#include "qemu_boot.h"
#include "qemu_serial.h"
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

#define _NSECTIONS(b)        (((b)+0x000fffff) >> 20)

#define QEMU_INTMEM_MMUFLAGS  MMU_MEMFLAGS
#define QEMU_PERIPH_MMUFLAGS  MMU_IOFLAGS

#define QEMU_INTMEM_NSECTIONS _NSECTIONS(CONFIG_RAMBANK1_SIZE)
#define QEMU_PERIPH_NSECTIONS _NSECTIONS(CONFIG_DEVICEIO_SIZE)
/****************************************************************************
 * Public Data
 ****************************************************************************/

extern uint32_t _vector_start; /* Beginning of vector block */
extern uint32_t _vector_end;   /* End+1 of vector block */

volatile uint32_t* g_current_regs[CONFIG_SMP_NCPUS];
#define CURRENT_REGS (g_current_regs[up_cpu_index()])

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This table describes how to map a set of 1Mb pages to space the physical
 * address space of the QEMU.
 */

#ifndef CONFIG_ARCH_ROMPGTABLE
static const struct section_mapping_s section_mapping[] =
{
#ifdef CONFIG_ARCH_LOWVECTORS
  { 0x40300000, QEMU_VECTOR_VADDR,  /* Map the vector table to lowmem */
    QEMU_INTMEM_MMUFLAGS,  1
  },
#endif
  { CONFIG_RAMBANK1_ADDR, CONFIG_RAMBANK1_ADDR,  /* Includes vectors and page table */
    QEMU_INTMEM_MMUFLAGS,  QEMU_INTMEM_NSECTIONS
  },
  { CONFIG_DEVICEIO_BASEADDR, CONFIG_DEVICEIO_BASEADDR,
    QEMU_PERIPH_MMUFLAGS,  QEMU_PERIPH_NSECTIONS
  },

};
#define NMAPPINGS \
  (sizeof(section_mapping) / sizeof(struct section_mapping_s))
#endif

volatile uint32_t *g_current_regs[CONFIG_SMP_NCPUS];


/****************************************************************************
 * Private Functions
 ****************************************************************************/


/****************************************************************************
 * Name: qemu_setupmappings
 *
 * Description:
 *   Map all of the initial memory regions defined in section_mapping[]
 *
 ****************************************************************************/

#ifndef CONFIG_ARCH_ROMPGTABLE
static inline void qemu_setupmappings(void)
{
  mmu_l1_map_regions(section_mapping, NMAPPINGS);
}
#endif

/****************************************************************************
 * Name: qemu_vectorpermissions
 *
 * Description:
 *   Set permissions on the vector mapping.
 *
 ****************************************************************************/

#if !defined(CONFIG_ARCH_ROMPGTABLE) && defined(CONFIG_ARCH_LOWVECTORS) && \
     defined(CONFIG_PAGING)
static void qemu_vectorpermissions(uint32_t mmuflags)
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
 * Name: qemu_vectormapping
 *
 * Description:
 *   Setup a special mapping for the interrupt vectors when (1) the
 *   interrupt vectors are not positioned in ROM, and when (2) the interrupt
 *   vectors are located at the high address, 0xffff0000.  When the
 *   interrupt vectors are located in ROM, we just have to assume that they
 *   were set up correctly;  When vectors  are located in low memory,
 *   0x00000000, the mapping for the ROM memory region will be suppressed.
 *
 ****************************************************************************/
#if !defined(CONFIG_ARCH_ROMPGTABLE) && !defined(CONFIG_ARCH_LOWVECTORS)
static void qemu_vectormapping(void)
{
  uint32_t vector_paddr = QEMU_VECTOR_PADDR & PTE_SMALL_PADDR_MASK;
  uint32_t vector_vaddr = QEMU_VECTOR_VADDR & PTE_SMALL_PADDR_MASK;
  uint32_t vector_size  = (uint32_t)&_vector_end - (uint32_t)&_vector_start;
  uint32_t end_paddr    = QEMU_VECTOR_PADDR + vector_size;

  /* REVISIT:  Cannot really assert in this context */

  DEBUGASSERT (vector_size <= VECTOR_TABLE_SIZE);

  /* We want to keep our interrupt vectors and interrupt-related logic in
   * zero-wait state internal SRAM (ISRAM).  The QEMU has 128Kb of ISRAM
   * positioned at physical address 0x0300:0000; we need to map this to
   * 0xffff:0000.
   */

  while (vector_paddr < end_paddr)
    {
      mmu_l2_setentry(VECTOR_L2_VBASE,  vector_paddr, vector_vaddr,
                      MMU_L2_VECTORFLAGS);
      vector_paddr += 4096;
      vector_vaddr += 4096;
    }

  /* Now set the level 1 descriptor to refer to the level 2 page table. */

  mmu_l1_setentry(VECTOR_L2_PBASE & PMD_PTE_PADDR_MASK,
                  QEMU_VECTOR_VADDR & PMD_PTE_PADDR_MASK,
                  MMU_L1_VECTORFLAGS);
}
#else
  /* No vector remap */

#  define qemu_vectormapping()
#endif

/****************************************************************************
 * Name: qemu_copyvectorblock
 *
 * Description:
 *   Copy the interrupt block to its final destination.  Vectors are already
 *   positioned at the beginning of the text region and only need to be
 *   copied in the case where we are using high vectors.
 *
 ****************************************************************************/

static void qemu_copyvectorblock(void)
{
  uint32_t *src;
  uint32_t *end;
  uint32_t *dest;

  /* If we are using re-mapped vectors in an area that has been marked
   * read only, then temporarily mark the mapping write-able (non-buffered).
   */

#ifdef CONFIG_PAGING
  qemu_vectorpermissions(MMU_L2_VECTRWFLAGS);
#endif

  /* Copy the vectors into ISRAM at the address that will be mapped to the
   * vector address:
   *
   *   QEMU_VECTOR_PADDR - Unmapped, physical address of vector table in SRAM
   *   QEMU_VECTOR_VSRAM - Virtual address of vector table in SRAM
   *   QEMU_VECTOR_VADDR - Virtual address of vector table (0x00000000 or
   *                      0xffff0000)
   */

  src  = (uint32_t *)&_vector_start;
  end  = (uint32_t *)&_vector_end;
  dest = (uint32_t *)(QEMU_VECTOR_VSRAM + VECTOR_TABLE_OFFSET);

  while (src < end)
    {
      *dest++ = *src++;
    }

  /* Make the vectors read-only, cacheable again */

#if !defined(CONFIG_ARCH_LOWVECTORS) && defined(CONFIG_PAGING)
  qemu_vectorpermissions(MMU_L2_VECTORFLAGS);
#endif
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm64_chip_boot
 *
 * Description:
 *   Complete boot operations started in arm64_head.S
 *
 ****************************************************************************/

void arm64_chip_boot(void)
{
  #ifndef CONFIG_ARCH_ROMPGTABLE
  /* __start provided the basic MMU mappings for SRAM.  Now provide mappings
   * for all IO regions (Including the vector region).
   */

  qemu_setupmappings();

  /* Provide a special mapping for the IRAM interrupt vector positioned in
   * high memory.
   */

  qemu_vectormapping();

#endif /* CONFIG_ARCH_ROMPGTABLE */

  /* Setup up vector block.  _vector_start and _vector_end are exported from
   * arm_vector.S
   */

//  qemu_copyvectorblock();

  /* Initialize the FPU */

  arm_fpuconfig();


#ifdef CONFIG_SMP
  arm64_psci_init("smc");

#endif

#ifdef USE_EARLYSERIALINIT
  /* Perform early serial initialization if we are going to use the serial
   * driver.
   */

  qemu_earlyserialinit();
#endif
}

#if defined(CONFIG_NET) && !defined(CONFIG_NETDEV_LATEINIT)
void arm64_netinitialize(void)
{
  /* TODO: qemu net support will be support */
}
#endif

static void boot_early_memset(void *dst, int c, size_t n)
{
  uint8_t *d = dst;

  while (n--)
    {
      *d++ = c;
    }
}

void arm_boot(void)
{
  //boot_early_memset(_START_BSS, 0, _END_BSS - _START_BSS);
  arm64_chip_boot();
  //nx_start();
}


void up_irqinitialize(void)
{
  /* The following operations need to be atomic, but since this function is
   * called early in the initialization sequence, we expect to have exclusive
   * access to the GIC.
   */

  /* Initialize the Generic Interrupt Controller (GIC) for CPU0 */

//  arm_gic_initialize();   /* Initialization common to all CPUs */

  /* currents_regs is non-NULL only while processing an interrupt */

  CURRENT_REGS = NULL;

#ifdef CONFIG_SMP
  arm64_smp_sgi_init();
#endif

#ifndef CONFIG_SUPPRESS_INTERRUPTS

  /* And finally, enable interrupts */

  up_irq_enable();
#endif
}


void up_timer_initialize(void)
{
	
}

