/****************************************************************************
 * arch/arm/src/armv7-a/up_fpuconfig.S
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

#include <nuttx/config.h>
#include "cp15.h"

#ifdef CONFIG_ARCH_FPU

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

	.globl	up_fpuconfig
	.file	"up_fpuconfig.S"

/****************************************************************************
 * Assembly Macros
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

	.text
	.syntax	unified
	.arm

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_fpuconfig
 *
 * Description:
 *   Configure the FPU.  Enables access to CP10 and CP11
 *
 ****************************************************************************/

	.globl	up_fpuconfig
	.type	up_fpuconfig, %function

up_fpuconfig:

	/* Enable access to CP10 and CP11 in CP15.CACR */

	mrc		CP15_CPACR(r0)
	orr		r0, r0, #0xf00000
	mcr		CP15_CPACR(r0)

	/* Enable access to CP10 and CP11 in CP15.NSACR */
	/* REVISIT: Do we need to do this? */

	/* Set FPEXC.EN (B30) */

	fmrx		r0, fpexc
	orr		r0, r0, #0x40000000
	fmxr		fpexc, r0
	bx		lr
	.size	up_fpuconfig, . - up_fpuconfig
#endif
	.end
