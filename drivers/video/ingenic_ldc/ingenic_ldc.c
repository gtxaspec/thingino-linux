/*
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Input file for Ingenic LDC driver
 *
 * This  program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <soc/base.h>
#include "ingenic_ldc.h"
#include "ingenic_lut.h"

//#define TIMEOUT_TEST
#define ldc_debug_pix
//#define DEBUG

#ifdef	DEBUG
static int debug_ldc = 1;
#define LDC_DEBUG(format, ...) { if (debug_ldc) printk(format, ## __VA_ARGS__);}
#else
#define LDC_DEBUG(format, ...) do{ } while(0)
#endif

#define LDC_BUF_SIZE (1024 * 1024 * 2)
//extern tx_isp_ldc_opt ldc_default_params[];

static int sheet = 5;
struct ldc_reg_struct jz_ldc_regs_name[] = {
};


static void reg_bit_set(struct jz_ldc *ldc, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg = reg_read(ldc, offset);
	reg |= bit;
	reg_write(ldc, offset, reg);

}

/*
static void reg_bit_clr(struct jz_ldc *ldc, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg= reg_read(ldc, offset);
	reg &= ~(bit);
	reg_write(ldc, offset, reg);

}
*/


/*
static int _ldc_dump_regs(struct jz_ldc *ldc)
{
	return 0;
}

static void _ldc_dump_param(struct jz_ldc *ldc)
{
	return;
}

static int ldc_dump_info(struct jz_ldc *ldc)
{
	int ret = 0;
	if (ldc == NULL) {
		dev_err(ldc->dev, "ldc is NULL\n");
		return -1;
	}
	printk("ldc: ldc->base: %p\n", ldc->iomem);
	_ldc_dump_param(ldc);
	ret = _ldc_dump_regs(ldc);

	return ret;
}
*/

static int reg_map_init(struct jz_ldc *ldc)
{
	//int ret = 0;
	int num = 0;
	LDC_DEBUG("(%s, %d)------>.\n",__func__,__LINE__);
	//Y
	for(num = 0;num < 0xff;num++)
	{
		reg_write(ldc, 0x1000 + num*4, ldc_default_params[sheet].y_shift_lut[num]);
		reg_write(ldc, 0x3000 + num*4, ldc_default_params[sheet].uv_shift_lut[num]);
        /*
        if(sheet == 0){
            if(num == 3){
                reg_write(ldc, 0x1000 + num*4, 0x14d);
                reg_write(ldc, 0x3000 + num*4, 0xa4);
            }else if(num == 5){
                reg_write(ldc, 0x1000 + num*4, 0x128);
                reg_write(ldc, 0x3000 + num*4, 0x92);
            }else if(num == 7){
                reg_write(ldc, 0x1000 + num*4, 0xe6);
                reg_write(ldc, 0x3000 + num*4, 0x71);
            }else if(num == 8){
                reg_write(ldc, 0x1000 + num*4, 0x19b);
                reg_write(ldc, 0x3000 + num*4, 0xcb);
            }else if(num == 10){
                reg_write(ldc, 0x1000 + num*4, 0x108);
                reg_write(ldc, 0x3000 + num*4, 0x83);
            }else if(num == 12){
                reg_write(ldc, 0x1000 + num*4, 0xea);
                reg_write(ldc, 0x3000 + num*4, 0x74);
            }else if(num == 14){
                reg_write(ldc, 0x1000 + num*4, 0x1a8);
                reg_write(ldc, 0x3000 + num*4, 0xd7);
            }else{
                reg_write(ldc, 0x1000 + num*4, 0x0);
                reg_write(ldc, 0x3000 + num*4, 0x0);
            }
        }else{
            if(num == 3){
                reg_write(ldc, 0x1000 + num*4, 0x15f);
                reg_write(ldc, 0x3000 + num*4, 0xad);
            }else if(num == 5){
                reg_write(ldc, 0x1000 + num*4, 0xf5);
                reg_write(ldc, 0x3000 + num*4, 0x79);
            }else if(num == 6){
                reg_write(ldc, 0x1000 + num*4, 0x152);
                reg_write(ldc, 0x3000 + num*4, 0xa7);
            }else if(num == 8){
                reg_write(ldc, 0x1000 + num*4, 0xe5);
                reg_write(ldc, 0x3000 + num*4, 0x71);
            }else if(num == 9){
                reg_write(ldc, 0x1000 + num*4, 0x153);
                reg_write(ldc, 0x3000 + num*4, 0xa8);
            }else if(num == 11){
                reg_write(ldc, 0x1000 + num*4, 0xd4);
                reg_write(ldc, 0x3000 + num*4, 0x6a);
            }else if(num == 12){
                reg_write(ldc, 0x1000 + num*4, 0x161);
                reg_write(ldc, 0x3000 + num*4, 0xb0);
            }else if(num == 14){
                reg_write(ldc, 0x1000 + num*4, 0xa8);
                reg_write(ldc, 0x3000 + num*4, 0x53);
            }else if(num == 15){
                reg_write(ldc, 0x1000 + num*4, 0x163);
                reg_write(ldc, 0x3000 + num*4, 0xb2);
            }else if(num == 17){
                reg_write(ldc, 0x1000 + num*4, 0xe6);
                reg_write(ldc, 0x3000 + num*4, 0x73);
            }else{
                reg_write(ldc, 0x1000 + num*4, 0x0);
                reg_write(ldc, 0x3000 + num*4, 0x0);
            }
        }
        */
	}
	return 0;

}

static int set_ldc_init(struct jz_ldc *ldc, struct ldc_param *ldc_init)
{
    int ret = 0;
    unsigned int value;
    unsigned int offset = ldc_default_params[sheet].width * ldc_default_params[sheet].height;
    LDC_DEBUG("(%s, %d)------>.\n",__func__,__LINE__);

    /*
    if(sheet == 0){
        ldc_init->k1 = 0x66;
        ldc_init->k2 = 0x133;
        ldc_init->k3 = 0x890;
        ldc_init->k4 = 0x890;
    }else{
        ldc_init->k1 = 0xa3;
        ldc_init->k2 = 0x1ae;
        ldc_init->k3 = 0x8cd;
        ldc_init->k4 = 0x8cd;
    }
    */

    if(ldc_default_params ==  NULL){
        printk("ldc_default_params is NULL !!!\n");
        return -1;
    }


    if(ldc_default_params[sheet].width != ldc_init->src_w || ldc_default_params[sheet].height != ldc_init->src_h){
        printk("resolution set err (%d,%d),driver resolution is (%d,%d)\n",ldc_init->src_w, ldc_init->src_h,ldc_default_params[sheet].width, ldc_default_params[sheet].height);
        return -1;
    }

    if(PIX_FMT_NV12 != ldc_init->data_type){
        printk("data type set err, only support nv12 !!!\n");
        return -1;
    }


    ret = reg_map_init(ldc);
    if(ret < 0){
        printk("init reg err ! [%d]\n",__LINE__);
    }

    value = reg_read(ldc, LDC_ADDR_VERSION);
    LDC_DEBUG("vresion is 0x%x\n",value);

    //set
	reg_write(ldc, LDC_ADDR_RES_SIZE, ((ldc_default_params[sheet].height << 16) | (ldc_default_params[sheet].width)));
	//reg_write(ldc, LDC_ADDR_X_CORF, 0x00660133);
	reg_write(ldc, LDC_ADDR_X_CORF, (ldc_default_params[sheet].k2_x << 16 | ldc_default_params[sheet].k1_x));
	//reg_write(ldc, LDC_ADDR_BROCF, 0x08900890);
	reg_write(ldc, LDC_ADDR_BROCF,  (ldc_default_params[sheet].k1_y << 16 | ldc_default_params[sheet].k2_y));
	reg_write(ldc, LDC_ADDR_R2_REP, ldc_default_params[sheet].r2_rep);
	reg_write(ldc, LDC_ADDR_UDIS_R, 0x0);

    //set Y
    LDC_DEBUG("-->0x%x,0x%x\n",ldc_init->in_addr,ldc_init->out_addr);
    reg_write(ldc, LDC_ADDR_Y_DMAIN_ADDR, ldc_init->in_addr);
    reg_write(ldc, LDC_ADDR_Y_DMAIN_STRIDE, ldc_default_params[sheet].y_str);
    reg_write(ldc, LDC_ADDR_Y_DMAOT_ADDR, ldc_init->out_addr);
    reg_write(ldc, LDC_ADDR_Y_DMAOT_STRIDE, ldc_default_params[sheet].y_str);

    //set uv
    reg_write(ldc, LDC_ADDR_UV_DMAIN_ADDR, ldc_init->in_addr + offset);
    reg_write(ldc, LDC_ADDR_UV_DMAIN_STRIDE, ldc_default_params[sheet].uv_str);
    reg_write(ldc, LDC_ADDR_UV_DMAOT_ADDR, ldc_init->out_addr + offset);
    reg_write(ldc, LDC_ADDR_UV_DMAOT_STRIDE, ldc_default_params[sheet].uv_str);

    reg_write(ldc, LDC_ADDR_LEN_COEF, ldc_default_params[sheet].len_cofe);
    return ret;
}


static  int set_ldc_dma_enable(struct jz_ldc *ldc, struct ldc_initarg *ldc_enable)
{
    int ret = 0;
    if(ldc_enable->on){
        //enable
        LDC_DEBUG("(%s, %d)dma enable.\n",__func__,__LINE__);
        ldc->streaming = 1;
       reg_write(ldc, LDC_ADDR_LDC_WORK_EN, 0x1);
    }else{
        ldc->streaming = 0;
        printk("ldc stream off.\n");
    }

    return ret;
}

/*
static int ldc_reg_set(struct jz_ldc *ldc, struct ldc_param *ldc_param)
{


    return 0;
}
*/

/*
static int ldc_start(struct jz_ldc *ldc, struct ldc_param *ldc_param)
{
	return 0;

}
*/

static int ldc_listen_buffer(struct jz_ldc *ldc, unsigned long arg)
{
    int ret = 0;
    struct ldc_initarg ldc_dbg;
    LDC_DEBUG("(%s, %d)------>.\n",__func__,__LINE__);
	mutex_lock(&ldc->irq_mutex);

#ifdef ldc_debug_pix
	ret = wait_for_completion_interruptible_timeout(&ldc->done_ldc, msecs_to_jiffies(15*1000));
	if (ret < 0) {
	    ldc_dbg.on = 0xfe;
		printk("ldc: done_ldc wait_for_completion_interruptible_timeout err %d\n", ret);
		return -1;
	} else if (ret == 0) {
		ret = -1;
	    __reset_ldc(0x1);
		printk("ldc: done_ldc wait_for_completion_interruptible_timeout timeout %d\n", ret);
		//ldc_dump_info(ldc);
		ldc_dbg.buf = ret;
	    ldc_dbg.on = 0xff;
	    mutex_unlock(&ldc->irq_mutex);
		return -1;
	} else {
		ldc_dbg.buf = 1;
	    ldc_dbg.on = reg_read(ldc, LDC_ADDR_DEBUG);
	}


    LDC_DEBUG("-->%s,%d\n",__func__,ldc_dbg.on);
    ret = copy_to_user((void __user *)arg, &ldc_dbg, sizeof(ldc_dbg));
    if (ret){
        printk("copy_to_user error!!\n");
        return -1;
    }

#else

	ret = wait_for_completion_interruptible(&ldc->done_ldc);
	if (ret ==  0) {
		ldc_dbg.buf = ret;
	    ldc_dbg.on = reg_read(ldc, LDC_ADDR_DEBUG);
	    mutex_unlock(&ldc->irq_mutex);
	} else {
        ldc->streaming = 0;
	    __reset_ldc(0x1);
	    ldc_dbg.on = 0xfe;
		printk("ldc: done_ldc wait_for_completion_interruptible_timeout err %d\n", ret);
		return -1;
	}

    LDC_DEBUG("-->%s,%d\n",__func__,ldc_dbg.on);
    ret = copy_to_user((void __user *)arg, &ldc_dbg, sizeof(ldc_dbg));
    if (ret){
        printk("copy_to_user error!!\n");
        return -1;
    }

#endif
	mutex_unlock(&ldc->irq_mutex);
    return ret;
}

void ldcgettimee(int64_t *ptime)
{
    struct timeval sttime;
    do_gettimeofday(&sttime);
    *ptime = sttime.tv_sec  * 1000000 + (sttime.tv_usec);
    return ;
}


static long ldc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct ldc_param iparam;
	struct miscdevice *dev = filp->private_data;
	struct jz_ldc *ldc = container_of(dev, struct jz_ldc, misc_dev);
    //int64_t time0 = 0,time1 = 0,time2 = 0;
    struct ldc_initarg ldc_enable;
	//LDC_DEBUG("ldc: %s pid: %d, tgid: %d file: %p, cmd: 0x%08x\n",
			//__func__, current->pid, current->tgid, filp, cmd);

	if (_IOC_TYPE(cmd) != JZLDC_IOC_MAGIC) {
		dev_err(ldc->dev, "invalid cmd!\n");
		return -EFAULT;
	}

	mutex_lock(&ldc->mutex);
   switch(cmd){
        case IOCTL_LDC_RES_PBUFF:
            if(copy_from_user(&ldc_enable,(void *)arg,sizeof(struct ldc_initarg))){
				printk("copy_from_user error!!!\n");
				ret = -EFAULT;
				break;
            }
            set_ldc_dma_enable(ldc, &ldc_enable);
            break;
        case IOCTL_LDC_START:
            if(copy_from_user( &iparam, (void *)arg, sizeof(struct ldc_param))){
				printk("copy_from_user error!!!\n");
				ret = -EFAULT;
				break;
            }
            sheet = iparam.ldc_default_params_num;
            set_ldc_init(ldc, &iparam);
            break;
		case IOCTL_LDC_LISTEN:
			    ldc_listen_buffer(ldc, arg);
            break;
		case IOCTL_LDC_BUF_FLUSH_CACHE:
			{
				struct ldc_flush_cache_para fc;
				if (copy_from_user(&fc, (void *)arg, sizeof(fc))) {
					dev_err(ldc->dev, "copy_from_user error!!!\n");
					ret = -EFAULT;
					break;
				}
				//dma_sync_single_for_device(NULL, fc.addr, fc.size, DMA_TO_DEVICE);
				//dma_sync_single_for_device(NULL, fc.addr, fc.size, DMA_FROM_DEVICE);
				dma_cache_sync(NULL, fc.addr, fc.size, DMA_BIDIRECTIONAL);
			}
			break;
        default:
            printk("%d,%s\n",__LINE__,__func__);
            break;
    }

	mutex_unlock(&ldc->mutex);
	return ret;
}

static int ldc_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_ldc *ldc = container_of(dev, struct jz_ldc, misc_dev);

	LDC_DEBUG("ldc: %s pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&ldc->mutex);

	mutex_unlock(&ldc->mutex);
	return ret;
}

static int ldc_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_ldc *ldc = container_of(dev, struct jz_ldc, misc_dev);

	LDC_DEBUG("ldc: %s  pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&ldc->mutex);

	mutex_unlock(&ldc->mutex);
	return ret;
}

static struct file_operations ldc_ops = {
	.owner = THIS_MODULE,
	.open = ldc_open,
	.release = ldc_release,
	.unlocked_ioctl = ldc_ioctl,
};

//int ttt = 0;

static irqreturn_t ldc_irq_handler(int irq, void *data)
{
	struct jz_ldc *ldc;
	unsigned int status,mask;

	//LDC_DEBUG("ldc: %s\n", __func__);
	LDC_DEBUG("(%s, %d)------>.\n",__func__,__LINE__);
	ldc = (struct jz_ldc *)data;

    /*
    if(sheet == 0)
        sheet = 1;
    else
        sheet = 0;
    */

    status = reg_read(ldc, LDC_ADDR_INT_STATE);
    mask = reg_read(ldc, LDC_ADDR_INT_MASK);

    __ldc_irq_clear(status);

    LDC_DEBUG("----- %s, status= 0x%08x,sheet = %d\n", __func__, status,sheet);
	 /* this status doesn't do anything including trigger interrupt,
	 * just give a hint */
    if (status & (~mask & 0x02)){
        printk("handler time out ...\n");
        complete(&ldc->done_ldc);
    }

    //if (status & 0x1){
    if (status & (~mask & 0x01)){
        complete(&ldc->done_ldc);
    }
    /*
    set_ldc_init(ldc, &iparam);
    struct ldc_initarg ldc_enable;
    ldc_enable.on = 1;
    set_ldc_dma_enable(ldc, &ldc_enable);
    */
#ifndef ldc_debug_pix
   if(ldc->streaming)
       reg_write(ldc, LDC_ADDR_LDC_WORK_EN, 0x1);
#endif
    /*timeout 10 times,let timeout value 0xffffffff,this can normal run handler*/
   // if (status & 0x2){
        //if(ttt > 10){
        //    reg_write(ldc, LDC_TIMEOUT_VALUE, 0xfffffff); //timeout value
        //}else{
        //    ttt++;
        //}
    //}
    return IRQ_HANDLED;
}

static int ldc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_ldc *ldc;
    int rc = 0;

	LDC_DEBUG("%s\n", __func__);
	ldc = (struct jz_ldc *)devm_kzalloc(&pdev->dev, sizeof(struct jz_ldc), GFP_KERNEL);
	if (!ldc) {
		dev_err(&pdev->dev, "alloc jz_ldc failed!\n");
		return -ENOMEM;
	}

	sprintf(ldc->name, "ldc");

	ldc->misc_dev.minor = MISC_DYNAMIC_MINOR;
	ldc->misc_dev.name = ldc->name;
	ldc->misc_dev.fops = &ldc_ops;
	ldc->dev = &pdev->dev;

	mutex_init(&ldc->mutex);
	mutex_init(&ldc->irq_mutex);
	init_completion(&ldc->done_ldc);
	init_completion(&ldc->done_buf);
	complete(&ldc->done_buf);

	ldc->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!ldc->res) {
		dev_err(&pdev->dev, "failed to get dev resources: %d\n", ret);
		ret = -EINVAL;
	}

    pdev->id = of_alias_get_id(pdev->dev.of_node, "ldc");

    ldc->iomem = devm_ioremap_resource(&pdev->dev, ldc->res);
    if (IS_ERR(ldc->iomem)) {
        dev_err(&pdev->dev, "failed to map io baseaddress. \n");
        ret = -ENODEV;
        goto err_ioremap;
    }

	ldc->irq = platform_get_irq(pdev, 0);
	if (devm_request_irq(&pdev->dev, ldc->irq, ldc_irq_handler, IRQF_SHARED, ldc->name, ldc)) {
		dev_err(&pdev->dev, "request irq failed\n");
		ret = -EINVAL;
		goto err_req_irq;
	}

	ldc->clk = devm_clk_get(ldc->dev, "gate_ldc");
	if (IS_ERR(ldc->clk)) {
		dev_err(&pdev->dev, "ldc clk get failed!\n");
		goto err_get_ldc_clk;
	}

	ldc->ahb0_gate = devm_clk_get(ldc->dev, "gate_ahb0");
	if (IS_ERR(ldc->clk)) {
		dev_err(&pdev->dev, "ldc clk get failed!\n");
		goto err_get_ahb_clk;
	}


    if ((rc = clk_prepare_enable(ldc->clk)) < 0) {
		dev_err(&pdev->dev, "Enable ldc gate clk failed\n");
		goto err_prepare_ldc_clk;
	}
	if ((rc = clk_prepare_enable(ldc->ahb0_gate)) < 0) {
		dev_err(&pdev->dev, "Enable ldc ahb0 clk failed\n");
		goto err_prepare_ahb_clk;
	}

    dev_set_drvdata(&pdev->dev, ldc);

	//__reset_safe_ldc();

    ret = misc_register(&ldc->misc_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "register misc device failed!\n");
		goto err_set_drvdata;
	}

	return 0;

err_set_drvdata:
	devm_free_irq(&pdev->dev, ldc->irq, ldc);
err_get_ldc_clk:
	devm_clk_put(&pdev->dev, ldc->clk);
err_get_ahb_clk:
	devm_clk_put(&pdev->dev, ldc->ahb0_gate);
err_prepare_ldc_clk:
	clk_disable_unprepare(ldc->clk);
err_prepare_ahb_clk:
	clk_disable_unprepare(ldc->ahb0_gate);
err_req_irq:
    devm_free_irq(&pdev->dev, ldc->irq, ldc);
err_ioremap:
	devm_iounmap(&pdev->dev, ldc->iomem);
	kfree(ldc);

	return ret;
}

static int ldc_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_ldc *ldc;
	LDC_DEBUG("%s\n", __func__);

	ldc = dev_get_drvdata(&pdev->dev);

    free_irq(ldc->irq, ldc);

    devm_iounmap(&pdev->dev, ldc->iomem);

	misc_deregister(&ldc->misc_dev);
	if (ret < 0) {
		dev_err(ldc->dev, "misc_deregister error %d\n", ret);
		return ret;
	}

    if (ldc->pbuf.vaddr_alloc) {
		kfree((void *)(ldc->pbuf.vaddr_alloc));
		ldc->pbuf.vaddr_alloc = 0;
	}

    clk_disable_unprepare(ldc->clk);

	clk_disable_unprepare(ldc->ahb0_gate);

	if (ldc) {
		kfree(ldc);
	}

	return 0;
}

static const struct of_device_id ingenic_ldc_dt_match[] = {
	{ .compatible = "ingenic,t40-ldc", .data = NULL },
	{ .compatible = "ingenic,t41-ldc", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_ldc_dt_match);

static struct platform_driver jz_ldc_driver = {
	.probe	= ldc_probe,
	.remove = ldc_remove,
	.driver = {
		.name = "jz-ldc",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(ingenic_ldc_dt_match),
	},
};

module_platform_driver(jz_ldc_driver)

MODULE_DESCRIPTION("JZ LDC driver");
MODULE_AUTHOR("jiansheng.zhang <jiansheng.zhang@ingenic.cn>");
MODULE_LICENSE("GPL");
