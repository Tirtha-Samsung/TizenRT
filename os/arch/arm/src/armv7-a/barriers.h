/****************************************************************************
 * arch/arm/src/armv7-a/barriers.h
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

#ifndef __ARCH_ARM_SRC_ARMV7_A_BARRIERS_H
#define __ARCH_ARM_SRC_ARMV7_A_BARRIERS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ARMv7-A memory barriers */

#define up_isb(n) __asm__ __volatile__ ("isb " #n : : : "memory")
#define up_dsb(n) __asm__ __volatile__ ("dsb " #n : : : "memory")
#define up_dmb(n) __asm__ __volatile__ ("dmb " #n : : : "memory")
#define up_nop()  __asm__ __volatile__ ("nop\n")
#define up_sev()  __asm__ __volatile__ ("sev\n")

#define ARM_DSB()  up_dsb(15)
#define ARM_ISB()  up_isb(15)
#define ARM_DMB()  up_dmb(15)
#define ARM_NOP()  up_nop()
#define ARM_SEV()  up_sev()

#endif /* __ARCH_ARM_SRC_ARMV7_A_BARRIERS_H */
