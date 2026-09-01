


#if defined __UITRON || defined __ECOS

#include "ddr_arb.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"
static const DRV_SEM v_sem[] = {SEMID_ARB};

#elif defined(__FREERTOS)
#include <kwrap/error_no.h>
#include <kwrap/semaphore.h>
#include <kwrap/nvt_type.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>

#include <kwrap/task.h>
#include <string.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "dma_protected.h"
#include "cache_protected.h"
#include "interrupt.h"


static SEM_HANDLE *v_sem[1];
static SEM_HANDLE SEMID_OTP;
#else
#include "otp_platform.h"
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <mach/fmem.h>

UINT32 IOADDR_EFUSE_REG_BASE;
static struct semaphore otp_sem;
#endif




#if 0
void ddr_arb_platform_clk_enable(void)
{
#if defined __UITRON || defined __ECOS
	pll_enableClock(v_clken[0]);
#else
#endif
}

void ddr_arb_platform_clk_disable(void)
{
#if defined __UITRON || defined __ECOS
	pll_disableClock(v_clken[0]);
#else
#endif
}

void ddr_arb_platform_clk_get_freq(UINT32 *p_freq)
{
#if defined __UITRON || defined __ECOS
#else
#endif
}

void ddr_arb_platform_flg_clear(FLGPTN flg)
{
	clr_flg(v_flg_id[0], flg);
}

void ddr_arb_platform_flg_set(FLGPTN flg)
{
	 iset_flg(v_flg_id[0], flg);
}

void ddr_arb_platform_flg_wait(FLGPTN flg)
{
	FLGPTN              ptn;

//	printk("%s: flg id %d\r\n", __func__, v_flg_id[id]);
	wai_flg(&ptn, v_flg_id[0], flg, TWF_ORW | TWF_CLR);
//	printk("%s: done\r\n", __func__);
}

FLGPTN ddr_arb_platform_flg_check(FLGPTN chkptn)
{
	return kchk_flg(FLG_ID_DDR_ARB, chkptn);
}
#endif
ER otp_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[0]);
#elif defined(__FREERTOS)
	return SEM_WAIT(*v_sem[0]);
#else
    down(&otp_sem);
	return E_OK;
#endif
}

ER otp_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[0]);
#elif defined(__FREERTOS)
	SEM_SIGNAL(*v_sem[0]);
	return E_OK;
#else
    up(&otp_sem);
	return E_OK;
#endif
}

void otp_platform_delay_ms(UINT32 ms)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayMs(ms);
#elif defined __FREERTOS
	vos_task_delay_ms(ms);
#else
	msleep(ms);
#endif
}
#if defined __FREERTOS
void otp_platform_create_resource(void)
#else
void otp_platform_create_resource(MODULE_INFO *pmodule_info)
#endif
{
#if defined __FREERTOS
   	SEM_CREATE(SEMID_OTP, 1);
	v_sem[0] = &SEMID_OTP;
#else
	IOADDR_EFUSE_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	sema_init(&otp_sem, 1);
#endif
}

void otp_platform_release_resource(void)
{
#if defined __FREERTOS
	SEM_DESTROY(SEMID_OTP);
#endif
}

#if 0
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
static int is_create = 0;
irqreturn_t ddr_platform_isr(int irq, void *devid)
{
	arb_isr();
	return IRQ_HANDLED;
}
irqreturn_t ddr_platform_isr2(int irq, void *devid)
{
	arb2_isr();
	return IRQ_HANDLED;
}
void ddr_arb_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB);
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB_IST);
		SEM_CREATE(SEMID_DDR_ARB, 1);
		v_sem[0] = &SEMID_DDR_ARB;
		v_flg_id[0] = FLG_ID_DDR_ARB;
		vk_spin_lock_init(&v_spin_lock[0]);

		dma_set_monitor_period();

		request_irq(INT_ID_DMA, ddr_platform_isr, IRQF_TRIGGER_HIGH, "dma", 0);
		request_irq(INT_ID_DMA2, ddr_platform_isr2, IRQF_TRIGGER_HIGH, "dma2", 0);

		dma_set_system_priority(TRUE);

		is_create = 1;
	}
}
void ddr_arb_platform_release_resource(void)
{
	rel_flg(FLG_ID_DDR_ARB);
	rel_flg(FLG_ID_DDR_ARB_IST);
	SEM_DESTROY(SEMID_DDR_ARB);
}

#else

void ddr_arb_platform_set_ist_event(UINT32 events)
{
#if defined __UITRON || defined __ECOS
	drv_setIstEvent(DRV_IST_MODULE_DDR_ARB, events);
#else
	UINT32 losing_event;

//	printk("%s: 0x%x\r\n", __func__, events);

	losing_event = ist_ddr_arb_events & events;

	if (losing_event) {
		nvt_dbg(ERR, "losing event: now 0x%x, last 0x%x\r\n",
				events, ist_ddr_arb_events);
	}

	ist_ddr_arb_events |= events;
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
	iset_flg(FLG_ID_TMR_IST, DDR_ARB_IST_FLG_TRIG);
#else
	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(p_ddr_arb_tasklet);
#endif

#endif
}

#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
static THREAD_DECLARE(ddr_arb_platform_ist, arglist)
{
//	THREAD_ENTRY();

	while (!THREAD_SHOULD_STOP) {
		FLGPTN ptn;
		UINT32 flg = DDR_ARB_IST_FLG_EXIT|DDR_ARB_IST_FLG_TRIG;

		wai_flg(&ptn, FLG_ID_TMR_IST, flg, TWF_ORW | TWF_CLR);
//		printk("%s: ptn 0x%x\r\n", __func__, ptn);

		if (ptn & DDR_ARB_IST_FLG_TRIG) {
			UINT32 i, events;

			ddr_arb_platform_spin_lock();
			events = ist_ddr_arb_events;
			ist_ddr_arb_events = 0;
			ddr_arb_platform_spin_unlock();

			do {
				i = __builtin_ctz(events);
				events &= ~(1 << i);

				if (pf_ist_cbs_ddr_arb[i] != NULL) {
					pf_ist_cbs_ddr_arb[i](0);
				} else {
					printk("%s: %d is NULL, event 0x%x\r\n",
						__func__,
						i, events);
				}
			} while (events);
		}

		if (ptn & DDR_ARB_IST_FLG_EXIT) {
			printk("%s: thread exit\r\n", __func__);
			break;
		}
	}

	THREAD_RETURN(0);
}
#else //if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_TASKLET)
int ddr_arb_platform_ist(UINT32 event)
{
	UINT32 i, events;
	UINT32 spin_flags;

	spin_flags = ddr_arb_platform_spin_lock();
	events = ist_ddr_arb_events;
	ist_ddr_arb_events = 0;
	ddr_arb_platform_spin_unlock(spin_flags);

	do {
		i = __builtin_ctz(events);
		events &= ~(1 << i);

		if (pf_ist_cbs_ddr_arb[i] != NULL) {
			pf_ist_cbs_ddr_arb[i](0);
		} else {
			printk("%s: %d is NULL, event 0x%x\r\n",
				__func__,
				i, events);
		}
	} while (events);


	return 0;
}
#endif
#endif


#endif
#endif
