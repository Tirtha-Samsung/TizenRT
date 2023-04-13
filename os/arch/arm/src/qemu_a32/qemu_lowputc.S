/****************************************************************************
 * arch/arm64/src/qemu/qemu_lowputc.S
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
 ****************************************************************************
 *
 *    DESCRIPTION
 *       Wrapper for early printk
 *
 ***************************************************************************/

#include <tinyara/config.h>

//#include "macro.inc"

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

    .file    "qemu_lowputc.S"

/****************************************************************************
 * Assembly Macros
 ****************************************************************************/

 /* 32-bit register definition for qemu pl011 uart */

 #define UART1_BASE_ADDRESS 0x9000000
 #define EARLY_UART_PL011_BAUD_RATE  115200

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* PL011 UART initialization
 * xb: register which contains the UART base address
 * c: scratch register number
 */

	.text
	.globl up_earlyserialinit
	.type	up_earlyserialinit, %function

up_earlyserialinit:
    ldr   r1, =UART1_BASE_ADDRESS
    mov   r0, #(7372800 / EARLY_UART_PL011_BAUD_RATE % 16)
    strh  r0, [r1, #0x28]      /* -> UARTFBRD (Baud divisor fraction) */
    mov   r0, #(7372800 / EARLY_UART_PL011_BAUD_RATE / 16)
    strh  r0, [r1, #0x24]      /* -> UARTIBRD (Baud divisor integer) */
    mov   r0, #0x60             /* 8n1 */
    str   r0, [r1, #0x2C]      /* -> UARTLCR_H (Line control) */
    ldr   r0, =0x00000301       /* RXE | TXE | UARTEN */
    str   r0, [r1, #0x30]      /* -> UARTCR (Control Register) */
    bx lr
	.size	up_earlyserialinit, . - up_earlyserialinit


/* PL011 UART wait UART to be ready to transmit
 * xb: register which contains the UART base address
 * c: scratch register number
 */

	.macro early_uart_ready, xb, wt
1:
    ldrh  \wt, [\xb, #0x18]      /* <- UARTFR (Flag register) */
    tst   \wt, #0x8              /* Check BUSY bit */
    bne  1b                     /* Wait for the UART to be ready */
	.endm

/* PL011 UART transmit character
 * xb: register which contains the UART base address
 * wt: register which contains the character to transmit
 */

	.macro early_uart_transmit, xb, wt
    strb  \wt, [\xb]             /* -> UARTDR (Data Register) */
	.endm

/* Print a character on the UART - this function is called by C
 * r0: character to print
 */

	.text
	.globl	arm_lowputc
	.type	arm_lowputc, %function

arm_lowputc:
    ldr   r1, =UART1_BASE_ADDRESS
    early_uart_ready r1, r2
    early_uart_transmit r1, r0
    bx lr
	.size	arm_lowputc, . - arm_lowputc
