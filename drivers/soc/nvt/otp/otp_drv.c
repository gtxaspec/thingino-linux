#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "otp_drv.h"
//#include "otp_ioctl.h"
#include "otp_dbg.h"
#include "otp_platform.h"
//#include "ddr_arb_int.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ddr_arb_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_ddr_arb_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ddr_arb_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ddr_arb_drv_isr(int irq, void *devid);
irqreturn_t nvt_ddr_arb2_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_otp_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_otp_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_otp_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

//	init_waitqueue_head(&pmodule_info->xxx_wait_queue);
//	spin_lock_init(&pmodule_info->xxx_spinlock);
//	sema_init(&pmodule_info->xxx_sem, 1);
//	init_completion(&pmodule_info->xxx_completion);
//	tasklet_init(&pmodule_info->xxx_tasklet, nvt_ddr_arb_drv_do_tasklet, (unsigned long)pmodule_info);

	/* allocate graphic resource here */
    otp_platform_create_resource(pmodule_info);

	//printk("%s: resource done\r\n", __func__);


	/* initial clock here */

	/* Add HW Module initialization here when driver loaded */



	/* Add error handler here */

	return err;
}

int nvt_otp_drv_remove(MODULE_INFO *pmodule_info)
{


	/* Add HW Moduel release operation here*/

	/* release OS resources */
    otp_platform_release_resource();


	return 0;
}

int nvt_otp_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_otp_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_otp_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg)
{
	int err = 0;
#if 0
	int __user *argp = (int __user *)arg;

	//nvt_dbg(IND, "IF-%d cmd:%x\n =>%08X", if_id, cmd,DDR_ARB_IOC_CHKSUM);

	switch (cmd) {
	case DDR_ARB_IOC_CHKSUM: {
			ARB_CHKSUM param = {0};
			if (unlikely(copy_from_user(&param, argp, sizeof(param)))) {
				DBG_ERR("failed to copy_from_user\r\n");
				return -EFAULT;
			}
			if (param.version != DDR_ARB_IOC_VERSION) {
				DBG_ERR("version not matched kernel(%08X), user(%08X)\r\n", DDR_ARB_IOC_VERSION, param.version);
				return -EFAULT;
			}
			param.sum = arb_chksum(param.ddr_id, param.phy_addr, param.len);
			if (unlikely(copy_to_user(argp, &param, sizeof(param)))) {
                                DBG_ERR("failed to copy_to_user\r\n");
                                return -EFAULT;
                        }
		} break;
	}
#endif
	return err;
}
#if 0
irqreturn_t nvt_ddr_arb_drv_isr(int irq, void *devid)
{
	arb_isr();

	/* simple triggle and response mechanism*/
//	complete(&pmodule_info->xxx_completion);


	/*  Tasklet for bottom half mechanism */
//	tasklet_schedule(&pmodule_info->xxx_tasklet);

	return IRQ_HANDLED;
}

irqreturn_t nvt_ddr_arb2_drv_isr(int irq, void *devid)
{
	arb2_isr();

	/* simple triggle and response mechanism*/
//	complete(&pmodule_info->xxx_completion);


	/*  Tasklet for bottom half mechanism */
//	tasklet_schedule(&pmodule_info->xxx_tasklet);

	return IRQ_HANDLED;
}

int nvt_ddr_arb_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->xxx_completion);
	return 0;
}



void nvt_ddr_arb_drv_do_tasklet(unsigned long data)
{
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_TASKLET)
	ddr_arb_platform_ist(data);
#endif
}
#endif
