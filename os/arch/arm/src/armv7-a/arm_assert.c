/****************************************************************************
 *
 * Copyright 2023 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * arch/arm/src/armv7-a/arm_assert.c
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/arch.h>
#include <tinyara/board.h>
#include <tinyara/syslog/syslog.h>
#include <tinyara/usb/usbdev_trace.h>

#include <arch/board/board.h>

#include "sched/sched.h"
#include "irq/irq.h"
#include "arm_internal.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
extern int g_irq_nums[3];
char assert_info_str[CONFIG_STDIO_BUFFER_SIZE] = {'\0', };
bool abort_mode = false;

/* USB trace dumping */

#ifndef CONFIG_USBDEV_TRACE
#  undef CONFIG_ARCH_USBDUMP
#endif

#ifndef CONFIG_BOARD_RESET_ON_ASSERT
#  define CONFIG_BOARD_RESET_ON_ASSERT 0
#endif

#ifdef CONFIG_ARCH_STACKDUMP

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_stackdump
 ****************************************************************************/

static void arm_stackdump(uint32_t sp, uint32_t stack_top)
{
  uint32_t stack;

  /* Flush any buffered SYSLOG data to avoid overwrite */

  //syslog_flush();

  for (stack = sp & ~0x1f; stack < (stack_top & ~0x1f); stack += 32)
    {
      uint32_t *ptr = (uint32_t *)stack;
      _alert("%08" PRIx32 ": %08" PRIx32 " %08" PRIx32 " %08" PRIx32
             " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32
             " %08" PRIx32 "\n",
             stack, ptr[0], ptr[1], ptr[2], ptr[3],
             ptr[4], ptr[5], ptr[6], ptr[7]);
    }
}

/****************************************************************************
 * Name: arm_registerdump
 ****************************************************************************/

static void arm_registerdump(volatile uint32_t *regs)
{
  /* Dump the interrupt registers */

  _alert("R0: %08" PRIx32 " R1: %08" PRIx32
         " R2: %08" PRIx32 "  R3: %08" PRIx32 "\n",
         regs[REG_R0], regs[REG_R1], regs[REG_R2], regs[REG_R3]);
#ifdef CONFIG_ARM_THUMB
  _alert("R4: %08" PRIx32 " R5: %08" PRIx32
         " R6: %08" PRIx32 "  FP: %08" PRIx32 "\n",
         regs[REG_R4], regs[REG_R5], regs[REG_R6], regs[REG_R7]);
  _alert("R8: %08" PRIx32 " SB: %08" PRIx32
         " SL: %08" PRIx32 " R11: %08" PRIx32 "\n",
         regs[REG_R8], regs[REG_R9], regs[REG_R10], regs[REG_R11]);
#else
  _alert("R4: %08" PRIx32 " R5: %08" PRIx32
         " R6: %08" PRIx32 "  R7: %08" PRIx32 "\n",
         regs[REG_R4], regs[REG_R5], regs[REG_R6], regs[REG_R7]);
  _alert("R8: %08" PRIx32 " SB: %08" PRIx32
         " SL: %08" PRIx32 "  FP: %08" PRIx32 "\n",
         regs[REG_R8], regs[REG_R9], regs[REG_R10], regs[REG_R11]);
#endif
  _alert("IP: %08" PRIx32 " SP: %08" PRIx32
         " LR: %08" PRIx32 "  PC: %08" PRIx32 "\n",
         regs[REG_R12], regs[REG_R13], regs[REG_R14], regs[REG_R15]);

#if defined(REG_BASEPRI)
  _alert("xPSR: %08" PRIx32 " BASEPRI: %08" PRIx32
         " CONTROL: %08" PRIx32 "\n",
         regs[REG_XPSR], regs[REG_BASEPRI], getcontrol());
#elif defined(REG_PRIMASK)
  _alert("xPSR: %08" PRIx32 " PRIMASK: %08" PRIx32
         " CONTROL: %08" PRIx32 "\n",
         regs[REG_XPSR], regs[REG_PRIMASK], getcontrol());
#elif defined(REG_CPSR)
  _alert("CPSR: %08" PRIx32 "\n", regs[REG_CPSR]);
#endif

#ifdef REG_EXC_RETURN
  _alert("EXC_RETURN: %08" PRIx32 "\n", regs[REG_EXC_RETURN]);
#endif
}

/****************************************************************************
 * Name: arm_dump_task
 ****************************************************************************/

static void arm_dump_task(struct tcb_s *tcb, void *arg)
{
  char args[64] = "";
#ifdef CONFIG_STACK_COLORATION
  uint32_t stack_filled = 0;
  uint32_t stack_used;
#endif
#ifdef CONFIG_SCHED_CPULOAD
  struct cpuload_s cpuload;
  uint32_t fracpart;
  uint32_t intpart;
  uint32_t tmp;

  clock_cpuload(tcb->pid, 0, &cpuload);

  if (cpuload.total > 0)
    {
      tmp      = (1000 * cpuload.active) / cpuload.total;
      intpart  = tmp / 10;
      fracpart = tmp - 10 * intpart;
    }
  else
    {
      intpart  = 0;
      fracpart = 0;
    }
#endif

#ifdef CONFIG_STACK_COLORATION
  stack_used = up_check_tcbstack(tcb);
  if (tcb->adj_stack_size > 0 && stack_used > 0)
    {
      /* Use fixed-point math with one decimal place */

      stack_filled = 10 * 100 * stack_used / tcb->adj_stack_size;
    }
#endif

#ifndef CONFIG_DISABLE_PTHREAD
  if ((tcb->flags & TCB_FLAG_TTYPE_MASK) == TCB_FLAG_TTYPE_PTHREAD)
    {
      FAR struct pthread_tcb_s *ptcb = (FAR struct pthread_tcb_s *)tcb;

      snprintf(args, sizeof(args), " %p", ptcb->arg);
    }
  else
#endif
/*    {
      FAR char **argv = tcb->group->tg_info->argv + 1;
      size_t npos = 0;

      while (*argv != NULL && npos < sizeof(args))
        {
          npos += snprintf(args + npos, sizeof(args) - npos, " %s", *argv++);
        }
    }
*/
  /* Dump interesting properties of this task */

  _alert("  %4d   %4d"
#ifdef CONFIG_SMP
         "  %4d"
#endif
         "   %7lu"
#ifdef CONFIG_STACK_COLORATION
         "   %7lu   %3" PRId32 ".%1" PRId32 "%%%c"
#endif
#ifdef CONFIG_SCHED_CPULOAD
         "   %3" PRId32 ".%01" PRId32 "%%"
#endif
         "   %s%s\n"
         , tcb->pid, tcb->sched_priority
#ifdef CONFIG_SMP
         , tcb->cpu
#endif
         , (unsigned long)tcb->adj_stack_size
#ifdef CONFIG_STACK_COLORATION
         , (unsigned long)up_check_tcbstack(tcb)
         , stack_filled / 10, stack_filled % 10
         , (stack_filled >= 10 * 80 ? '!' : ' ')
#endif
#ifdef CONFIG_SCHED_CPULOAD
         , intpart, fracpart
#endif
#if CONFIG_TASK_NAME_SIZE > 0
         , tcb->name
#else
         , "<noname>"
#endif
         , args
        );
}

/****************************************************************************
 * Name: arm_dump_backtrace
 ****************************************************************************/

#ifdef CONFIG_SCHED_BACKTRACE
static void arm_dump_backtrace(struct tcb_s *tcb, void *arg)
{
  /* Show back trace */

  sched_dumpstack(tcb->pid);
}
#endif

/****************************************************************************
 * Name: arm_showtasks
 ****************************************************************************/

static void arm_showtasks(void)
{
#if CONFIG_ARCH_INTERRUPTSTACK > 7
#  ifdef CONFIG_STACK_COLORATION
  uint32_t stack_used = up_check_intstack();
  uint32_t stack_filled = 0;

  if ((CONFIG_ARCH_INTERRUPTSTACK & ~7) > 0 && stack_used > 0)
    {
      /* Use fixed-point math with one decimal place */

      stack_filled = 10 * 100 *
                     stack_used / (CONFIG_ARCH_INTERRUPTSTACK & ~7);
    }
#  endif
#endif

  /* Dump interesting properties of each task in the crash environment */

  _alert("   PID    PRI"
#ifdef CONFIG_SMP
         "   CPU"
#endif
         "     STACK"
#ifdef CONFIG_STACK_COLORATION
         "      USED   FILLED "
#endif
#ifdef CONFIG_SCHED_CPULOAD
         "      CPU"
#endif
         "   COMMAND\n");

#if CONFIG_ARCH_INTERRUPTSTACK > 7
  _alert("  ----   ----"
#  ifdef CONFIG_SMP
         "  ----"
#  endif
         "   %7u"
#  ifdef CONFIG_STACK_COLORATION
         "   %7" PRId32 "   %3" PRId32 ".%1" PRId32 "%%%c"
#  endif
#  ifdef CONFIG_SCHED_CPULOAD
         "     ----"
#  endif
         "   irq\n"
         , (CONFIG_ARCH_INTERRUPTSTACK & ~7)
#  ifdef CONFIG_STACK_COLORATION
         , stack_used
         , stack_filled / 10, stack_filled % 10,
         (stack_filled >= 10 * 80 ? '!' : ' ')
#  endif
        );
#endif

  sched_foreach(arm_dump_task, NULL);
#ifdef CONFIG_SCHED_BACKTRACE
 sched_foreach(arm_dump_backtrace, NULL);
#endif
}

/****************************************************************************
 * Name: assert_tracecallback
 ****************************************************************************/

#ifdef CONFIG_ARCH_USBDUMP
static int usbtrace_syslog(const char *fmt, ...)
{
  va_list ap;

  /* Let vsyslog do the real work */

  va_start(ap, fmt);
  vsyslog(LOG_EMERG, fmt, ap);
  va_end(ap);
  return OK;
}

static int assert_tracecallback(struct usbtrace_s *trace, void *arg)
{
  usbtrace_trprintf(usbtrace_syslog, trace->event, trace->value);
  return 0;
}
#endif

/****************************************************************************
 * Name: arm_dump_stack
 ****************************************************************************/

static void arm_dump_stack(const char *tag, uint32_t sp,
                           uint32_t base, uint32_t size, bool force)
{
  uint32_t top = base + size;

  _alert("%s Stack:\n", tag);
  _alert("sp:     %08" PRIx32 "\n", sp);
  _alert("  base: %08" PRIx32 "\n", base);
  _alert("  size: %08" PRIx32 "\n", size);

  if (sp >= base && sp < top)
    {
      arm_stackdump(sp, top);
    }
  else
    {
      _alert("ERROR: %s Stack pointer is not within the stack\n", tag);
      if (force)
        {
#ifdef CONFIG_STACK_COLORATION
          uint32_t remain;

          remain = size - arm_stack_check((FAR void *)(uintptr_t)base, size);
          base  += remain;
          size  -= remain;
#endif

#if CONFIG_ARCH_STACKDUMP_MAX_LENGTH > 0
          if (size > CONFIG_ARCH_STACKDUMP_MAX_LENGTH)
            {
              size = CONFIG_ARCH_STACKDUMP_MAX_LENGTH;
            }
#endif

          arm_stackdump(base, base + size);
        }
    }
}

/****************************************************************************
 * Name: arm_dumpstate
 ****************************************************************************/

static void arm_dumpstate(void)
{
  struct tcb_s *rtcb = this_task();
  uint32_t sp = up_getsp();
  uint32_t stackbase = 0;
  uint32_t stacksize = 0;
#if CONFIG_ARCH_INTERRUPTSTACK > 3
  uint32_t istackbase = 0;
  uint32_t istacksize = 0;
#endif
  uint32_t nestirqstkbase = 0;
  uint32_t nestirqstksize = 0;
  uint8_t irq_num;
  
  /* Get the limits for each type of stack */

  stackbase = (uint32_t)rtcb->adj_stack_ptr;
  stacksize = (uint32_t)rtcb->adj_stack_size;

#ifdef CONFIG_ARCH_NESTED_IRQ_STACK_SIZE
	nestirqstkbase = (uint32_t)&g_nestedirqstkbase;
	nestirqstksize = (CONFIG_ARCH_NESTED_IRQ_STACK_SIZE & ~3);
#endif
	bool is_irq_assert = false;
	bool is_sp_corrupt = false;

	/* Check if the assert location is in user thread or IRQ handler.
	 * If the irq_num is lesser than NVIC_IRQ_USAGEFAULT, then it is
	 * a fault and not an irq.
	 */
	if (g_irq_nums[2] && (g_irq_nums[0] <= NVIC_IRQ_USAGEFAULT)) {
		/* Assert in nested irq */
		irq_num = 1;
		is_irq_assert = true;
		lldbg("Code asserted in nested IRQ state!\n");
	} else if (g_irq_nums[1] && (g_irq_nums[0] > NVIC_IRQ_USAGEFAULT)) {
		/* Assert in nested irq */
		irq_num = 0;
		is_irq_assert = true;
		lldbg("Code asserted in nested IRQ state!\n");
	} else if (g_irq_nums[1] && (g_irq_nums[0] <= NVIC_IRQ_USAGEFAULT)) {
		/* Assert in irq */
		irq_num = 1;
		is_irq_assert = true;
		lldbg("Code asserted in IRQ state!\n");
	} else if (g_irq_nums[0] > NVIC_IRQ_USAGEFAULT) {
		/* Assert in irq */
		irq_num = 0;
		is_irq_assert = true;
		lldbg("Code asserted in IRQ state!\n");
	} else {
		/* Assert in user thread */
		lldbg("Code asserted in normal thread!\n");
		if (CURRENT_REGS) {
			/* If assert is in user thread, but current_regs is not NULL,
			 * it means that assert happened due to a fault. So, we want to
			 * reset the sp to the value just before the fault happened
			 */
			sp = CURRENT_REGS[REG_R13];
		}
	}
  
/* Print IRQ handler details if required */

	if (is_irq_assert) {
		lldbg("IRQ num: %d\n", g_irq_nums[irq_num]);
		lldbg("IRQ handler: %08x \n", g_irqvector[g_irq_nums[irq_num]].handler);
#ifdef CONFIG_DEBUG_IRQ_INFO
		lldbg("IRQ name: %s \n", g_irqvector[g_irq_nums[irq_num]].irq_name);
#endif
		if ((sp <= nestirqstkbase) && (sp > (nestirqstkbase - nestirqstksize))) {
			stackbase = nestirqstkbase;
			stacksize = nestirqstksize;
			lldbg("Current SP is Nested IRQ SP: %08x\n", sp);
			lldbg("Nested IRQ stack:\n");
		} else
#if CONFIG_ARCH_INTERRUPTSTACK > 3
		if ((sp <= istackbase) && (sp > (istackbase - istacksize))) {
			stackbase = istackbase;
			stacksize = istacksize;
			lldbg("Current SP is IRQ SP: %08x\n", sp);
			lldbg("IRQ stack:\n");
		} else {
			is_sp_corrupt = true;
		}
#else
		if ((sp <= stackbase) && (sp > (stackbase - stacksize))) {
			lldbg("Current SP is User Thread SP: %08x\n", sp);
			lldbg("User stack:\n");
		} else {
			is_sp_corrupt = true;
		}
#endif
	} else if ((sp <= stackbase) && (sp > (stackbase - stacksize))) {
		lldbg("Current SP is User Thread SP: %08x\n", sp);
		lldbg("User stack:\n");
	} else {
		is_sp_corrupt = true;
	}


	if (is_sp_corrupt) {
		lldbg("ERROR: Stack pointer is not within any of the allocated stack\n");
		lldbg("Wrong Stack pointer %08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		sp, *((uint32_t *)sp + 0), *((uint32_t *)sp + 1), *((uint32_t *)sp + 2), ((uint32_t *)sp + 3),
		*((uint32_t *)sp + 4), ((uint32_t *)sp + 5), ((uint32_t *)sp + 6), ((uint32_t *)sp + 7));

		/* Since SP is corrupted, we dont know which stack was being used.
		 * So, dump all the available stacks.
		 */
#ifdef CONFIG_ARCH_NESTED_IRQ_STACK_SIZE
		lldbg("Nested IRQ stack dump:\n");
		arm_stackdump(nestirqstkbase - nestirqstksize + 1, nestirqstkbase);
#endif
#if CONFIG_ARCH_INTERRUPTSTACK > 3
		lldbg("IRQ stack dump:\n");
		arm_stackdump(istackbase - istacksize + 1, istackbase);
#endif
		lldbg("User thread stack dump:\n");
		arm_stackdump(stackbase - stacksize + 1, stackbase);
	} else {
		/* Dump the stack region which contains the current stack pointer */
		lldbg("  base: %08x\n", stackbase);
		lldbg("  size: %08x\n", stacksize);
#ifdef CONFIG_STACK_COLORATION
		lldbg("  used: %08x\n", up_check_assertstack((uintptr_t)(stackbase - stacksize), stacksize));
#endif
		arm_stackdump(sp, stackbase);
	}

   /* Show back trace */

#ifdef CONFIG_SCHED_BACKTRACE
  sched_dumpstack(rtcb->pid);
#endif

  /* Update the xcp context */

  if (CURRENT_REGS)
    {
      rtcb->xcp.regs = (uint32_t *)CURRENT_REGS;
    }
  else
    {
      up_saveusercontext(rtcb->xcp.regs);
    }

  /* Dump the registers */

  arm_registerdump(rtcb->xcp.regs);

  /* Dump the irq stack */

#if CONFIG_ARCH_INTERRUPTSTACK > 7
  arm_dump_stack("IRQ", sp,
#  ifdef CONFIG_SMP
                 (uint32_t)arm_intstack_alloc(),
#  else
                 (uint32_t)&g_intstackalloc,
#  endif
                 (CONFIG_ARCH_INTERRUPTSTACK & ~7),
                 !!CURRENT_REGS);

  if (CURRENT_REGS)
    {
      sp = CURRENT_REGS[REG_R13];
    }
#endif

  /* Dump the user stack */

  arm_dump_stack("User", sp,
                 (uint32_t)rtcb->stack_base_ptr,
                 (uint32_t)rtcb->adj_stack_size,
#ifdef CONFIG_ARCH_KERNEL_STACK
                 false
#else
                 true
#endif
                );

#ifdef CONFIG_ARCH_KERNEL_STACK
  arm_dump_stack("Kernel", sp,
                 (uint32_t)rtcb->xcp.kstack,
                 CONFIG_ARCH_KERNEL_STACKSIZE,
                 false);
#endif

  /* Dump the state of all tasks (if available) */

  arm_showtasks();

#ifdef CONFIG_ARCH_USBDUMP
  /* Dump USB trace data */

  usbtrace_enumerate(assert_tracecallback, NULL);
#endif
}
#endif /* CONFIG_ARCH_STACKDUMP */

/****************************************************************************
 * Name: arm_assert
 ****************************************************************************/

static void arm_assert(void)
{
  /* Flush any buffered SYSLOG data */

 // syslog_flush();

  /* Are we in an interrupt handler or the idle task? */

  if (CURRENT_REGS || (this_task())->flink == NULL)
    {
#if CONFIG_BOARD_RESET_ON_ASSERT >= 1
      board_reset(CONFIG_BOARD_ASSERT_RESET_VALUE);
#endif

      /* Disable interrupts on this CPU */

      irqsave();

#ifdef CONFIG_SMP
      /* Try (again) to stop activity on other CPUs */

      spin_trylock(&g_cpu_irqlock);
#endif

      for (; ; )
        {
#ifdef CONFIG_ARCH_LEDS
          /* FLASH LEDs a 2Hz */

          board_autoled_on(LED_PANIC);
          up_mdelay(250);
          board_autoled_off(LED_PANIC);
          up_mdelay(250);
#endif
        }
    }
  else
    {
#if CONFIG_BOARD_RESET_ON_ASSERT >= 2
      board_reset(CONFIG_BOARD_ASSERT_RESET_VALUE);
#endif
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_assert
 ****************************************************************************/

void up_assert(FAR const uint8_t *filename, int linenum)
{
  board_autoled_on(LED_ASSERTION);

  /* Flush any buffered SYSLOG data (prior to the assertion) */

  //syslog_flush();

  _alert("Assertion failed "
#ifdef CONFIG_SMP
         "CPU%d "
#endif
         "at file:%s line: %d"
#if CONFIG_TASK_NAME_SIZE > 0
         " task: %s"
#endif
         "\n",
#ifdef CONFIG_SMP
         up_cpu_index(),
#endif
         filename, linenum
#if CONFIG_TASK_NAME_SIZE > 0
         , this_task()->name
#endif
        );

  /* Print the extra arguments (if any) from ASSERT_INFO macro */
	if (assert_info_str[0]) {
		lldbg("%s\n", assert_info_str);
	}

#ifdef CONFIG_ARCH_STACKDUMP
  arm_dumpstate();
#endif

#ifdef CONFIG_APP_BINARY_SEPARATION
	elf_show_all_bin_section_addr();
#endif

  /* Flush any buffered SYSLOG data (from the above) */

  //syslog_flush();

#ifdef CONFIG_BOARD_CRASHDUMP
  board_crashdump(up_getsp(), this_task(), filename, linenum);
#endif

  arm_assert();
}
