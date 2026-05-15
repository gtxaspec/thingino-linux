#include <asm/processor.h>
#include <linux/sched.h>
#include <mxu.h>

void __save_mxu(void *tsk_void)
{
	struct task_struct *tsk = tsk_void;
	volatile register void *reg_val asm("t0");
#if 0
	reg_val = tsk->thread.mxu.regs;
	asm volatile(".set mips32r2\n\t"
               " sa1q $vr0, 0*16($8)\n\t"
               " sa1q $vr1, 1*16($8)\n\t"
               " sa1q $vr2, 2*16($8)\n\t"
               " sa1q $vr3, 3*16($8)\n\t"
               " sa1q $vr4, 4*16($8)\n\t"
               " sa1q $vr5, 5*16($8)\n\t"
               " sa1q $vr6, 6*16($8)\n\t"
               " sa1q $vr7, 7*16($8)\n\t"
               " sa1q $vr8, 8*16($8)\n\t"
               " sa1q $vr9, 9*16($8)\n\t"
               " sa1q $vr10, 10*16($8)\n\t"
               " sa1q $vr11, 11*16($8)\n\t"
               " sa1q $vr12, 12*16($8)\n\t"
               " sa1q $vr13, 13*16($8)\n\t"
               " sa1q $vr14, 14*16($8)\n\t"
               " sa1q $vr15, 15*16($8)\n\t"
               " sa1q $vr16, 16*16($8)\n\t"
               " sa1q $vr17, 17*16($8)\n\t"
               " sa1q $vr18, 18*16($8)\n\t"
               " sa1q $vr19, 19*16($8)\n\t"
               " sa1q $vr20, 20*16($8)\n\t"
               " sa1q $vr21, 21*16($8)\n\t"
               " sa1q $vr22, 22*16($8)\n\t"
               " sa1q $vr23, 23*16($8)\n\t"
               " sa1q $vr24, 24*16($8)\n\t"
               " sa1q $vr25, 25*16($8)\n\t"
               " sa1q $vr26, 26*16($8)\n\t"
               " sa1q $vr27, 27*16($8)\n\t"
               " sa1q $vr28, 28*16($8)\n\t"
               " sa1q $vr29, 29*16($8)\n\t"
               " sa1q $vr30, 30*16($8)\n\t"
               " sa1q $vr31, 31*16($8)\n\t"
               ".set mips32r2");
	reg_val = &tsk->thread.mxu.csr;
	asm volatile(".set mips32r2\n\t"
               " cfcmxu $9, $31\n\t"
               " sw $9, 0($8)\n\t"
               ".set mips32r2");

#else
    reg_val = (volatile char *)tsk->thread.mxu.regs;
	asm volatile("":"+r"(reg_val));
	asm volatile(".word	0x7100003c \n\t");
	asm volatile(".word	0x7100087c \n\t");
	asm volatile(".word	0x710010bc \n\t");
	asm volatile(".word	0x710018fc \n\t");
	asm volatile(".word	0x7100213c \n\t");
	asm volatile(".word	0x7100297c \n\t");
	asm volatile(".word	0x710031bc \n\t");
	asm volatile(".word	0x710039fc \n\t");
	asm volatile(".word	0x7100423c \n\t");
	asm volatile(".word	0x71004a7c \n\t");
	asm volatile(".word	0x710052bc \n\t");
	asm volatile(".word	0x71005afc \n\t");
	asm volatile(".word	0x7100633c \n\t");
	asm volatile(".word	0x71006b7c \n\t");
	asm volatile(".word	0x710073bc \n\t");
	asm volatile(".word	0x71007bfc \n\t");
	asm volatile(".word	0x7100843c \n\t");
	asm volatile(".word	0x71008c7c \n\t");
	asm volatile(".word	0x710094bc \n\t");
	asm volatile(".word	0x71009cfc \n\t");
	asm volatile(".word	0x7100a53c \n\t");
	asm volatile(".word	0x7100ad7c \n\t");
	asm volatile(".word	0x7100b5bc \n\t");
	asm volatile(".word	0x7100bdfc \n\t");
	asm volatile(".word	0x7100c63c \n\t");
	asm volatile(".word	0x7100ce7c \n\t");
	asm volatile(".word	0x7100d6bc \n\t");
	asm volatile(".word	0x7100defc \n\t");
	asm volatile(".word	0x7100e73c \n\t");
	asm volatile(".word	0x7100ef7c \n\t");
	asm volatile(".word	0x7100f7bc \n\t");
	asm volatile(".word	0x7100fffc \n\t");
	reg_val = (volatile char *)&tsk->thread.mxu.csr;
	asm volatile("":"+r"(reg_val));
	/*asm volatile(*/
               /*" lw $9, 0(%0)\n\t"*/
			/*::"r"(reg_val));*/
	asm volatile(".word	0x4bc14ffd \n\t");
	asm volatile(".word	0xad090000 \n\t");
#endif
}

void __restore_mxu(void * tsk_void)
{
	struct task_struct *tsk = tsk_void;
	volatile register void *reg_val asm("t0");
	reg_val = tsk->thread.mxu.regs;
	asm volatile("":"+r"(reg_val));
	/*asm volatile(*/
               /*" lw $9, 0(%0)\n\t"*/
			/*::"r"(reg_val));*/
#if 0
	asm volatile(".set mips32r2\n\t"
               " la1q $vr0, 0*16($8)\n\t"
               " la1q $vr1, 1*16($8)\n\t"
               " la1q $vr2, 2*16($8)\n\t"
               " la1q $vr3, 3*16($8)\n\t"
               " la1q $vr4, 4*16($8)\n\t"
               " la1q $vr5, 5*16($8)\n\t"
               " la1q $vr6, 6*16($8)\n\t"
               " la1q $vr7, 7*16($8)\n\t"
               " la1q $vr8, 8*16($8)\n\t"
               " la1q $vr9, 9*16($8)\n\t"
               " la1q $vr10, 10*16($8)\n\t"
               " la1q $vr11, 11*16($8)\n\t"
               " la1q $vr12, 12*16($8)\n\t"
               " la1q $vr13, 13*16($8)\n\t"
               " la1q $vr14, 14*16($8)\n\t"
               " la1q $vr15, 15*16($8)\n\t"
               " la1q $vr16, 16*16($8)\n\t"
               " la1q $vr17, 17*16($8)\n\t"
               " la1q $vr18, 18*16($8)\n\t"
               " la1q $vr19, 19*16($8)\n\t"
               " la1q $vr20, 20*16($8)\n\t"
               " la1q $vr21, 21*16($8)\n\t"
               " la1q $vr22, 22*16($8)\n\t"
               " la1q $vr23, 23*16($8)\n\t"
               " la1q $vr24, 24*16($8)\n\t"
               " la1q $vr25, 25*16($8)\n\t"
               " la1q $vr26, 26*16($8)\n\t"
               " la1q $vr27, 27*16($8)\n\t"
               " la1q $vr28, 28*16($8)\n\t"
               " la1q $vr29, 29*16($8)\n\t"
               " la1q $vr30, 30*16($8)\n\t"
               " la1q $vr31, 31*16($8)\n\t"
               ".set mips32r2");
	reg_val = &tsk->thread.mxu.csr;
	asm volatile(".set mips32r2\n\t"
               " lw $9, 0($8)\n\t"
               " ctcmxu $9, $31\n\t"
               ".set mips32r2");

#else
	asm volatile(".word	0x7100002c \n\t");
	asm volatile(".word	0x7100086c \n\t");
	asm volatile(".word	0x710010ac \n\t");
	asm volatile(".word	0x710018ec \n\t");
	asm volatile(".word	0x7100212c \n\t");
	asm volatile(".word	0x7100296c \n\t");
	asm volatile(".word	0x710031ac \n\t");
	asm volatile(".word	0x710039ec \n\t");
	asm volatile(".word	0x7100422c \n\t");
	asm volatile(".word	0x71004a6c \n\t");
	asm volatile(".word	0x710052ac \n\t");
	asm volatile(".word	0x71005aec \n\t");
	asm volatile(".word	0x7100632c \n\t");
	asm volatile(".word	0x71006b6c \n\t");
	asm volatile(".word	0x710073ac \n\t");
	asm volatile(".word	0x71007bec \n\t");
	asm volatile(".word	0x7100842c \n\t");
	asm volatile(".word	0x71008c6c \n\t");
	asm volatile(".word	0x710094ac \n\t");
	asm volatile(".word	0x71009cec \n\t");
	asm volatile(".word	0x7100a52c \n\t");
	asm volatile(".word	0x7100ad6c \n\t");
	asm volatile(".word	0x7100b5ac \n\t");
	asm volatile(".word	0x7100bdec \n\t");
	asm volatile(".word	0x7100c62c \n\t");
	asm volatile(".word	0x7100ce6c \n\t");
	asm volatile(".word	0x7100d6ac \n\t");
	asm volatile(".word	0x7100deec \n\t");
	asm volatile(".word	0x7100e72c \n\t");
	asm volatile(".word	0x7100ef6c \n\t");
	asm volatile(".word	0x7100f7ac \n\t");
	asm volatile(".word	0x7100ffec \n\t");
	reg_val = &tsk->thread.mxu.csr;
	asm volatile("":"+r"(reg_val));
	/*asm volatile(*/
               /*" lw $9, 0(%0)\n\t"*/
			/*::"r"(reg_val));*/
	asm volatile(".word	0x8d090000 \n\t");
	asm volatile(".word	0x4bc14ffc \n\t");
#endif
}
/*
 * Kernel-side mxuv support functions
 */
void kernel_mxu_begin_partial(unsigned int num_regs)
{
	/* if (in_interrupt()) { */
	/*     printk("%s in interrupt\n", __func__); */
	/*     BUG_ON(1); */
	/* } else { */
		/*
		 * Save the userland FPSIMD state if we have one and if we
		 * haven't done so already. Clear fpsimd_last_state to indicate
		 * that there is no longer userland FPSIMD state in the
		 * registers.
		 */
		preempt_disable();
        if(!(read_c0_status() & ST0_CU2)){
		    set_c0_status(ST0_CU2);
        }
		__save_mxu(current);
	/* } */
}
EXPORT_SYMBOL(kernel_mxu_begin_partial);

void kernel_mxu_end(void)
{
    unsigned int __c0_stat; 
	/* if (in_interrupt()) { */
	/*     printk("%s in interrupt\n", __func__); */
	/*     BUG_ON(1); */
	/* } else { */
		__restore_mxu(current);
        if(!(KSTK_STATUS(current) & ST0_CU2)) {
            __c0_stat = read_c0_status();
            write_c0_status(__c0_stat & ~ST0_CU2);
        }
		preempt_enable();
	/* } */
}
EXPORT_SYMBOL(kernel_mxu_end);
