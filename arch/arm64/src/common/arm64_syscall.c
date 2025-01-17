/****************************************************************************
 * arch/arm64/src/common/arm64_syscall.c
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

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <syscall.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <nuttx/addrenv.h>

#include "addrenv.h"
#include "arch/irq.h"
#include "arm64_internal.h"
#include "arm64_fatal.h"
#include "sched/sched.h"
#include "signal/signal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dispatch_syscall
 *
 * Description:
 *   Call the stub function corresponding to the system call.  NOTE the non-
 *   standard parameter passing:
 *
 *     x0 = SYS_ call number
 *     x1 = parm0
 *     x2 = parm1
 *     x3 = parm2
 *     x4 = parm3
 *     x5 = parm4
 *     x6 = parm5
 *
 *   The values of X4-X5 may be preserved in the proxy called by the user
 *   code if they are used (but otherwise will not be).
 *
 *   WARNING: There are hard-coded values in this logic!
 *
 ****************************************************************************/

#ifdef CONFIG_LIB_SYSCALL
//static void dispatch_syscall(void) naked_function;
static void dispatch_syscall(void)
{
  __asm__ __volatile__
  (
    " sub sp, sp, #8*8\n"           /* Create a stack frame to hold 7 parms + lr */
    " str x4, [sp, #0]\n"          /* Move parameter 4 (if any) into position */
    " str x5, [sp, #4]\n"          /* Move parameter 5 (if any) into position */
    " str x6, [sp, #8]\n"          /* Move parameter 6 (if any) into position */
    " str lr, [sp, #12]\n"         /* Save lr in the stack frame */
    " ldr x16, =g_stublookup\n"     /* R12=The base of the stub lookup table */
    " ldr x16, [x16, x0, lsl #3]\n"  /* R12=The address of the stub for this SYSCALL */
    " BLR X16\n"                    /* Call the stub (modifies lr) */
    " ldr lr, [sp, #12]\n"         /* Restore lr */
    " add sp, sp, #16\n"           /* Destroy the stack frame */
    " mov x2, x0\n"                /* R2=Save return value in R2 */
    " mov x0, %0\n"                /* R0=SYS_syscall_return */
    " svc %1\n"::"i"(SYS_syscall_return),
                 "i"(SYS_syscall)  /* Return from the SYSCALL */
  );
}
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void  arm64_dump_syscall(const char *tag, uint64_t cmd,
                                const struct regs_context * f_regs)
{
  svcinfo("SYSCALL %s: regs: %p cmd: %" PRId64 "\n", tag, f_regs, cmd);

  svcinfo("x0:  0x%-16lx  x1:  0x%lx\n",
          f_regs->regs[REG_X0], f_regs->regs[REG_X1]);
  svcinfo("x2:  0x%-16lx  x3:  0x%lx\n",
          f_regs->regs[REG_X2], f_regs->regs[REG_X3]);
  svcinfo("x4:  0x%-16lx  x5:  0x%lx\n",
          f_regs->regs[REG_X4], f_regs->regs[REG_X5]);
  svcinfo("x6:  0x%-16lx  x7:  0x%lx\n",
          f_regs->regs[REG_X6], f_regs->regs[REG_X7]);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm64_syscall_switch
 *
 * Description:
 *   task switch syscall
 *
 ****************************************************************************/

uint64_t *arm64_syscall_switch(uint64_t * regs)
{
  uint64_t             cmd;
  struct regs_context *f_regs;
  uint64_t            *ret_regs;

  /* Nested interrupts are not supported */

  DEBUGASSERT(regs);

  f_regs = (struct regs_context *)regs;

  /* The SYSCALL command is in x0 on entry.  Parameters follow in x1..x7 */

  cmd = f_regs->regs[REG_X0];

  arm64_dump_syscall(__func__, cmd, f_regs);

  switch (cmd)
    {
      /* x0 = SYS_restore_context:  Restore task context
       *
       * void arm64_fullcontextrestore(uint64_t *restoreregs)
       *   noreturn_function;
       *
       * At this point, the following values are saved in context:
       *
       *   x0 = SYS_restore_context
       *   x1 = restoreregs( xcp->regs, callee saved register save area)
       */

      case SYS_restore_context:
        {
          /* Replace 'regs' with the pointer to the register set in
           * regs[REG_R1].  On return from the system call, that register
           * set will determine the restored context.
           */

          ret_regs = (uint64_t *)f_regs->regs[REG_X1];
          f_regs->regs[REG_X1] = 0; /* set the saveregs = 0 */

          DEBUGASSERT(ret_regs);
        }
        break;

      /* x0 = SYS_switch_context:  This a switch context command:
       *
       * void arm64_switchcontext(uint64_t *saveregs, uint64_t *restoreregs);
       *
       * At this point, the following values are saved in context:
       *
       *   x0 = SYS_switch_context
       *   x1 = saveregs (xcp->regs, callee saved register save area)
       *   x2 = restoreregs (xcp->regs, callee saved register save area)
       *
       * In this case, we do both: We save the context registers to the save
       * register area reference by the saved contents of x1 and then set
       * regs to the save register area referenced by the saved
       * contents of x2.
       */

      case SYS_switch_context:
        {
          DEBUGASSERT(f_regs->regs[REG_X1] != 0 &&
                      f_regs->regs[REG_X2] != 0);
          *(uint64_t **)f_regs->regs[REG_X1] = regs;

          ret_regs = (uint64_t *) f_regs->regs[REG_X2];
        }
        break;

      default:
        {
          svcerr("ERROR: Bad SYS call: 0x%" PRIx64 "\n", cmd);
          ret_regs = 0;
          return 0;
        }
        break;
    }

  if ((uint64_t *)f_regs != ret_regs)
    {
#ifdef CONFIG_ARCH_ADDRENV
      /* Make sure that the address environment for the previously
       * running task is closed down gracefully (data caches dump,
       * MMU flushed) and set up the address environment for the new
       * thread at the head of the ready-to-run list.
       */

      addrenv_switch(NULL);
#endif

      /* Record the new "running" task.  g_running_tasks[] is only used by
       * assertion logic for reporting crashes.
       */

      g_running_tasks[this_cpu()] = this_task();

      /* Restore the cpu lock */

      restore_critical_section();
    }

  return ret_regs;
}

/****************************************************************************
 * Name: arm64_syscall
 *
 * Description:
 *   SVC interrupts will vector here with insn=the SVC instruction and
 *   xcp=the interrupt context
 *
 *   The handler may get the SVC number be de-referencing the return
 *   address saved in the xcp and decoding the SVC instruction
 *
 ****************************************************************************/

int arm64_syscall(uint64_t *regs)
{
  uint64_t             cmd;
  struct regs_context *f_regs;

  /* Nested interrupts are not supported */

  DEBUGASSERT(regs);

  f_regs = (struct regs_context *)regs;

  /* The SYSCALL command is in x0 on entry.  Parameters follow in x1..x7 */

  cmd = f_regs->regs[REG_X0];

  arm64_dump_syscall(__func__, cmd, f_regs);

  switch (cmd)
    {
      /* R0=SYS_syscall_return:  This a SYSCALL return command:
       *
       *   void arm_syscall_return(void);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_syscall_return
       *
       * We need to restore the saved return address and return in
       * unprivileged thread mode.
       */

#ifdef CONFIG_LIB_SYSCALL
      case SYS_syscall_return:
        {
          struct tcb_s *rtcb = nxsched_self();
          int index = (int)rtcb->xcp.nsyscalls - 1;

          /* Make sure that there is a saved SYSCALL return address. */

          DEBUGASSERT(index >= 0);

          /* Setup to return to the saved SYSCALL return address in
           * the original mode.
           */

          regs[REG_ELR]        = rtcb->xcp.syscall[index].sysreturn;
#ifdef CONFIG_BUILD_KERNEL
          regs[REG_SPSR]      = rtcb->xcp.syscall[index].cpsr;
#endif
          /* The return value must be in R0-R1.  dispatch_syscall()
           * temporarily moved the value for R0 into R2.
           */

          regs[REG_X0]         = regs[REG_X2];

#ifdef CONFIG_ARCH_KERNEL_STACK
          /* If this is the outermost SYSCALL and if there is a saved user
           * stack pointer, then restore the user stack pointer on this
           * final return to user code.
           */

          if (index == 0 && rtcb->xcp.ustkptr != NULL)
            {
              regs[REG_SP_ELX]      = (uint64_t)rtcb->xcp.ustkptr;
              rtcb->xcp.ustkptr = NULL;
            }
#endif

          /* Save the new SYSCALL nesting level */

          rtcb->xcp.nsyscalls   = index;

          /* Handle any signal actions that were deferred while processing
           * the system call.
           */

          rtcb->flags          &= ~TCB_FLAG_SYSCALL;
          (void)nxsig_unmask_pendingsignal();
        }
        break;
#endif

      /* R0=SYS_task_start:  This a user task start
       *
       *   void up_task_start(main_t taskentry, int argc, char *argv[])
       *     noreturn_function;
       *
       * At this point, the following values are saved in context:
       *
       *   X0 = SYS_task_start
       *   X1 = taskentry
       *   X2 = argc
       *   X3 = argv
       */

#ifdef CONFIG_BUILD_KERNEL
      case SYS_task_start:
        {
          /* Set up to return to the user-space _start function in
           * unprivileged mode.  We need:
           *
           *   R0   = argc
           *   R1   = argv
           *   PC   = taskentry
           *   CSPR = user mode
           */

          regs[REG_ELR]   = regs[REG_X1];
          regs[REG_X0]    = regs[REG_X2];
          regs[REG_X1]    = regs[REG_X3];

          regs[REG_SPSR]  = (regs[REG_SPSR] & ~SPSR_MODE_MASK) | SPSR_MODE_EL0T;
        }
        break;
#endif

      /* R0=SYS_pthread_start:  This a user pthread start
       *
       *   void up_pthread_start(pthread_startroutine_t entrypt,
       *                         pthread_addr_t arg) noreturn_function;
       *
       * At this point, the following values are saved in context:
       *
       *   X0 = SYS_pthread_start
       *   X1 = entrypt
       *   X2 = arg
       */

#if !defined(CONFIG_BUILD_FLAT) && !defined(CONFIG_DISABLE_PTHREAD)
      case SYS_pthread_start:
        {
          /* Set up to enter the user-space pthread start-up function in
           * unprivileged mode. We need:
           *
           *   R0   = entrypt
           *   R1   = arg
           *   PC   = startup
           *   CSPR = user mode
           */

          regs[REG_ELR]   = regs[REG_X1];
          regs[REG_X0]   = regs[REG_X2];
          regs[REG_X1]   = regs[REG_X3];

          regs[REG_SPSR] = (regs[REG_SPSR] & ~SPSR_MODE_MASK) | SPSR_MODE_EL0T;
        }
        break;
#endif

#ifdef CONFIG_BUILD_KERNEL
      /* R0=SYS_signal_handler:  This a user signal handler callback
       *
       * void signal_handler(_sa_sigaction_t sighand, int signo,
       *                     siginfo_t *info, void *ucontext);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_signal_handler
       *   R1 = sighand
       *   R2 = signo
       *   R3 = info
       *        ucontext (on the stack)
       */

      case SYS_signal_handler:
        {
          struct tcb_s *rtcb = nxsched_self();

          /* Remember the caller's return address */

          DEBUGASSERT(rtcb->xcp.sigreturn == 0);
          rtcb->xcp.sigreturn  = regs[REG_ELR];

          /* Set up to return to the user-space trampoline function in
           * unprivileged mode.
           */

          regs[REG_ELR]   = (uint64_t)ARCH_DATA_RESERVE->ar_sigtramp;
          regs[REG_SPSR] = (regs[REG_SPSR] & ~SPSR_MODE_MASK) | SPSR_MODE_EL0T;

          /* Change the parameter ordering to match the expectation of struct
           * userpace_s signal_handler.
           */

          regs[REG_X0]   = regs[REG_X1]; /* sighand */
          regs[REG_X1]   = regs[REG_X2]; /* signal */
          regs[REG_X2]   = regs[REG_X3]; /* info */
          regs[REG_X3]   = regs[REG_X4]; /* ucontext */

#ifdef CONFIG_ARCH_KERNEL_STACK
          /* If we are signalling a user process, then we must be operating
           * on the kernel stack now.  We need to switch back to the user
           * stack before dispatching the signal handler to the user code.
           * The existence of an allocated kernel stack is sufficient
           * information to make this decision.
           */

          if (rtcb->xcp.kstack != NULL)
            {
              uint64_t usp;

              DEBUGASSERT(rtcb->xcp.kstkptr == NULL);

              /* Copy "info" into user stack */

              if (rtcb->xcp.sigdeliver)
                {
                  usp = rtcb->xcp.saved_regs[REG_SP_ELX];
                }
              else
                {
                  usp = rtcb->xcp.regs[REG_SP_ELX];
                }

              /* Create a frame for info and copy the kernel info */

              usp = usp - sizeof(siginfo_t);
              memcpy((void *)usp, (void *)regs[REG_R2], sizeof(siginfo_t));

              /* Now set the updated SP and user copy of "info" to R2 */

              rtcb->xcp.kstkptr = (uint64_t *)regs[REG_SP_ELX];
              regs[REG_SP_ELX]      = usp;
              regs[REG_X2]      = usp;
            }
#endif
        }
        break;
#endif

#ifdef CONFIG_BUILD_KERNEL
      /* R0=SYS_signal_handler_return:  This a user signal handler callback
       *
       *   void signal_handler_return(void);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_signal_handler_return
       */

      case SYS_signal_handler_return:
        {
          struct tcb_s *rtcb = nxsched_self();

          /* Set up to return to the kernel-mode signal dispatching logic. */

          DEBUGASSERT(rtcb->xcp.sigreturn != 0);

          regs[REG_ELR]         = rtcb->xcp.sigreturn;
          regs[REG_SPSR] = (regs[REG_SPSR] & ~SPSR_MODE_MASK) | SPSR_MODE_EL1H;
          rtcb->xcp.sigreturn  = 0;

#ifdef CONFIG_ARCH_KERNEL_STACK
          /* We must enter here be using the user stack.  We need to switch
           * to back to the kernel user stack before returning to the kernel
           * mode signal trampoline.
           */

          if (rtcb->xcp.kstack != NULL)
            {
              DEBUGASSERT(rtcb->xcp.kstkptr != NULL);

              regs[REG_SP_ELX]      = (uint64_t)rtcb->xcp.kstkptr;
              rtcb->xcp.kstkptr = NULL;
            }
#endif
        }
        break;
#endif

      /* This is not an architecture-specific system call.  If NuttX is built
       * as a standalone kernel with a system call interface, then all of the
       * additional system calls must be handled as in the default case.
       */

      default:
        {
#ifdef CONFIG_LIB_SYSCALL
          struct tcb_s *rtcb = nxsched_self();
          int index = rtcb->xcp.nsyscalls;

          /* Verify that the SYS call number is within range */

          DEBUGASSERT(cmd >= CONFIG_SYS_RESERVED && cmd < SYS_maxsyscall);

          /* Make sure that there is a no saved SYSCALL return address.  We
           * cannot yet handle nested system calls.
           */

          DEBUGASSERT(index < CONFIG_SYS_NNEST);

          /* Setup to return to dispatch_syscall in privileged mode. */

          rtcb->xcp.syscall[index].sysreturn = regs[REG_ELR];
#ifdef CONFIG_BUILD_KERNEL
          rtcb->xcp.syscall[index].cpsr      = regs[REG_SPSR];
#endif

          regs[REG_ELR]   = (uint64_t)dispatch_syscall;
#ifdef CONFIG_BUILD_KERNEL
          regs[REG_SPSR] = (regs[REG_SPSR] & ~SPSR_MODE_MASK) | SPSR_MODE_EL1H;
#endif
          /* Offset R0 to account for the reserved values */

          regs[REG_X0]  -= CONFIG_SYS_RESERVED;

          /* Indicate that we are in a syscall handler. */

          rtcb->flags   |= TCB_FLAG_SYSCALL;

#ifdef CONFIG_ARCH_KERNEL_STACK
          /* If this is the first SYSCALL and if there is an allocated
           * kernel stack, then switch to the kernel stack.
           */

          if (index == 0 && rtcb->xcp.kstack != NULL)
            {
              rtcb->xcp.ustkptr = (uint64_t *)regs[REG_SP_ELX];
              if (rtcb->xcp.kstkptr != NULL)
                {
                  regs[REG_SP_ELX]  = (uint64_t)rtcb->xcp.kstkptr;
                }
              else
                {
                  regs[REG_SP_ELX]  = (uint64_t)rtcb->xcp.kstack +
                                  ARCH_KERNEL_STACKSIZE;
                }
            }
#endif

          /* Save the new SYSCALL nesting level */

          rtcb->xcp.nsyscalls   = index + 1;
#else
          svcerr("ERROR: Bad SYS call: 0x%" PRIx64 "\n", cmd);
#endif
        }
        break;
    }

  return 0;
}
