#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/bug.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include "tx-isp-device.h"
#include "tx-isp-interrupt.h"
#include "tx-isp-debug.h"
#include "tx-isp-vic.h"
#include "tx-isp-csi.h"
#include "tx-isp-video-in.h"
#include "apical-isp/tx-isp-core.h"
#include "fast_start_common.h"

static int tx_isp_media_pipeline_set_clk(struct tx_isp_media_pipeline *p, int state)
{
	struct isp_private_ioctl ioctl;
	struct v4l2_subdev *sd = NULL;
	int ret = 0,index = 0;

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_MODULE_CLK;
	ioctl.value = state;
	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}
	if(ret < 0 && ret != -ENOIOCTLCMD){
		ioctl.value = !state;
		while(--index >= 0){
			sd = p->subdevs[index];
			ret = v4l2_subdev_call(sd, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
		}
	}
	return ret;
}
static int tx_isp_media_pipeline_set_power(struct tx_isp_media_pipeline *p, int state)
{
	struct v4l2_subdev *sd = NULL;
	int ret = 0,index = 0;

	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, s_power, state);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}
	if(ret < 0 && ret != -ENOIOCTLCMD){
		while(--index >= 0){
			sd = p->subdevs[index];
			v4l2_subdev_call(sd, core, s_power, !state);
		}
	}
	return ret;
}
static int tx_isp_media_pipeline_prepare(struct tx_isp_media_pipeline *p) // struct tx_isp_video_in *vin)
{
	int ret = ISP_SUCCESS;
	if(!p)
		return -ISP_ERROR;
	ret = tx_isp_media_pipeline_set_clk(p, true);
	if(ret < 0)
		return -ISP_ERROR;
	ret = tx_isp_media_pipeline_set_power(p, true);
	if(ret != ISP_SUCCESS){
		tx_isp_media_pipeline_set_clk(p, false);
	}

	return ret;
}

static int tx_isp_media_pipeline_unprepare(struct tx_isp_media_pipeline *p)
{
	int ret = ISP_SUCCESS;
	if(!p)
		return ISP_SUCCESS;

	ret = tx_isp_media_pipeline_set_power(p, false);
	if(ret < 0)
		return -ISP_ERROR;
	tx_isp_media_pipeline_set_clk(p, false);

	return ISP_SUCCESS;
}

static int tx_isp_media_pipeline_reset(struct tx_isp_media_pipeline *p, int state)
{
	int ret = 0,index = 0;
	struct v4l2_subdev *sd = NULL;

	if(!p){
		return -ISP_ERROR;
	}

	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, reset, state);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}

	return ret;
}

static int tx_isp_media_pipeline_init(struct tx_isp_media_pipeline *p, int state)
{
	int ret = 0,index = 0;
	struct v4l2_subdev *sd = NULL;

	if(!p){
		return -ISP_ERROR;
	}

	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, init, state);
		if(ret < 0 && ret != -ENOIOCTLCMD){
			printk("^^^ %s[%d] name = %s ^^^\n", __func__,__LINE__, sd->name);
			break;
		}
		index++;
	}
	if(ret < 0){
		while(index >= 0){
			sd = p->subdevs[index];
			v4l2_subdev_call(sd, core, init, !state);
			index--;
		}
	}
	return ret;
}

static int tx_isp_media_pipeline_set_stream(struct tx_isp_media_pipeline *p, int state)
{
	struct v4l2_subdev *sd = NULL;
	int ret = 0,index = 0;

	if(!p){
		return -ISP_ERROR;
	}

	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, video, s_stream, state);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}
	if(ret < 0){
		while(index >= 0){
			sd = p->subdevs[index];
			v4l2_subdev_call(sd, video, s_stream, !state);
			index--;
		}
	}
	return ret;
}

struct tx_isp_media_pipeline_ops tx_isp_pipeline_ops = {
	.prepare = tx_isp_media_pipeline_prepare,
	.unprepare = tx_isp_media_pipeline_unprepare,
	.reset = tx_isp_media_pipeline_reset,
	.init = tx_isp_media_pipeline_init,
	.set_stream = tx_isp_media_pipeline_set_stream,
};

static int tx_isp_notify_sync_video_in(tx_isp_device_t* ispdev, int vin)
{
//	struct v4l2_device *v4l2_dev = &ispdev->v4l2_dev;
	struct tx_isp_media_pipeline *p = &ispdev->pipeline;
	struct v4l2_subdev *sd = NULL;
	struct isp_private_ioctl ioctl;
	int ret = 0,index = 0;

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_SYNC_VIDEO_IN;
	ioctl.value = vin;
	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}

	return ret;
}

static int tx_isp_notify_call_ioctrl(tx_isp_device_t* ispdev, void *arg)
{
//	struct v4l2_device *v4l2_dev = &ispdev->v4l2_dev;
	struct tx_isp_media_pipeline *p = &ispdev->pipeline;
	struct v4l2_subdev *sd = NULL;
	int ret = 0,index = 0;

	while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
		sd = p->subdevs[index];
		ret = v4l2_subdev_call(sd, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, arg);
		if(ret < 0 && ret != -ENOIOCTLCMD)
			break;
		index++;
	}

	return ret;
}


static int tx_isp_create_media_link_and_pipeline(tx_isp_device_t* ispdev)
{
	struct v4l2_device *v4l2_dev = &ispdev->v4l2_dev;
	struct tx_isp_media_pipeline *p = &ispdev->pipeline;
	struct v4l2_subdev *sd;
	struct media_entity *source, *link;
	unsigned int flags = 0;
	int index;
	int ret = ISP_SUCCESS;
	list_for_each_entry(sd, &v4l2_dev->subdevs, list) {
		if(sd->grp_id < TX_ISP_MAX_GRP_IDX)
			p->subdevs[sd->grp_id] = sd;
	}
	p->ops = &tx_isp_pipeline_ops;
	for(index = 0; index < TX_ISP_MAX_GRP_IDX - 1; index++){
		/*printk("&&&&%s[%d] index = %d  &&&&\n",__func__,__LINE__,index);*/
		source = &(p->subdevs[index]->entity);
		link = &(p->subdevs[index + 1]->entity);
		ret = media_entity_create_link(source, TX_ISP_PAD_SOURCE , link, TX_ISP_PAD_LINK, flags);
		if(ret < 0){
			v4l2_err(v4l2_dev, "Failed to create the link(%s => %s)!\n",
						source->name, link->name);
			ret = -ISP_ERROR;
			break;
		}
	}
	return ret;
}

#define sd_to_tx_ispdev(sd) (container_of((sd)->v4l2_dev, tx_isp_device_t, v4l2_dev))
static void tx_isp_v4l2_dev_notify(struct v4l2_subdev *sd, unsigned int notification, void *arg)
{
	tx_isp_device_t* ispdev = sd_to_tx_ispdev(sd);
	struct tx_isp_notify_argument *notify = (struct tx_isp_notify_argument *)arg;
//	printk("%s[%d] sd->name = %s\n",__func__, __LINE__, sd->name);
	switch(notification){
		case TX_ISP_NOTIFY_GET_PIPELINE:
			notify->value = (int)(&ispdev->pipeline);
			notify->ret = ISP_SUCCESS;
			break;
		case TX_ISP_NOTIFY_SYNC_VIDEO_IN:
			notify->ret = tx_isp_notify_sync_video_in(ispdev, notify->value);
			break;
		case TX_ISP_NOTIFY_ENABLE_IRQ:
			ispdev->irq_dev->enable_irq(ispdev->irq_dev, notify->value);
			notify->ret = ISP_SUCCESS;
			break;
		case TX_ISP_NOTIFY_DISABLE_IRQ:
			ispdev->irq_dev->disable_irq(ispdev->irq_dev, notify->value);
			notify->ret = ISP_SUCCESS;
			break;
		case TX_ISP_NOTIFY_MASK_IRQ:
			ispdev->irq_dev->mask_irq(ispdev->irq_dev, notify->value);
			notify->ret = ISP_SUCCESS;
			break;
		case TX_ISP_NOTIFY_UNMASK_IRQ:
			ispdev->irq_dev->unmask_irq(ispdev->irq_dev, notify->value);
			notify->ret = ISP_SUCCESS;
			break;
		case TX_ISP_NOTIFY_PRIVATE_IOCTL:
			notify->ret = tx_isp_notify_call_ioctrl(ispdev, (void *)notify->value);
			break;
		default:
			break;
	}
}
static int tx_isp_subdev_match(struct device *dev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	tx_isp_device_t* ispdev = (tx_isp_device_t*)data;
	int ret = ISP_SUCCESS;

	if(!get_device(dev))
		return -ENODEV;
	if(!strncmp(pdev->name, "tx-isp", 6)){
		if(!strcmp(pdev->name, TX_ISP_CORE_NAME))
			ret = register_tx_isp_core_device(pdev, &ispdev->v4l2_dev);
		else if(!strcmp(pdev->name, TX_ISP_VIC_NAME))
			ret = register_tx_isp_vic_device(pdev, &ispdev->v4l2_dev);
		else if(!strcmp(pdev->name, TX_ISP_CSI_NAME))
			ret = register_tx_isp_csi_device(pdev, &ispdev->v4l2_dev);
		else
			printk("register all isp device successfully!\n");
			/*printk("the device[%s] isn't belong to ISP platform device.\n", pdev->name);*/
	}
	put_device(dev);
	return ret;
}

static int tx_isp_md_link_notify(struct media_pad *source, struct media_pad *sink, u32 flags)
{
	printk("%s[%d] source is %s, sink is %s",__func__,__LINE__,
					source->entity->name, sink->entity->name);
	return ISP_SUCCESS;
}

static void tx_isp_release_subdevs(tx_isp_device_t* ispdev)
{
	struct v4l2_device *v4l2_dev = &ispdev->v4l2_dev;
	struct v4l2_subdev *sd;
	struct list_head *pos;
	int index = 0;

	list_for_each(pos, &v4l2_dev->subdevs) {
		sd = list_entry(pos, struct v4l2_subdev, list);
		switch(sd->grp_id){
			case  TX_ISP_CORE_GRP_IDX:
				release_tx_isp_core_device(sd);
				index++;
				break;
			case  TX_ISP_VIC_GRP_IDX:
				release_tx_isp_vic_device(sd);
				index++;
				break;
			case  TX_ISP_CSI_GRP_IDX:
				release_tx_isp_csi_device(sd);
				index++;
				break;
			case  TX_ISP_VIDEO_IN_GRP_IDX:
				release_tx_isp_video_in_device(sd);
				index++;
				break;
			default:
				break;

		}
		pos = &v4l2_dev->subdevs;
		if(index > TX_ISP_MAX_GRP_IDX){
			v4l2_info(v4l2_dev, "%s[%d] Failed to release isp's subdevs\n",__func__,__LINE__);
			break;
		}
	}
	v4l2_info(v4l2_dev, "Release all isp's subdevs\n");
}


#if TX_ISP_FAST_START
extern int isp_core_tuning_switch_day_or_night(int day);
extern int tx_isp_frame_channel_fast_start(void);
extern int g_is_day;
#endif

static int tx_isp_probe(struct platform_device *pdev)
{
	tx_isp_device_t* ispdev;
	struct v4l2_device *v4l2_dev;
	int ret = ISP_SUCCESS;
	/*printk("@@@@@@@@@@@@@@@@@@@@ tx-isp-probe @@@@@@@@@@@@@@@@@@@@@@@@@@\n");*/
	ispdev = (tx_isp_device_t*)kzalloc(sizeof(*ispdev), GFP_KERNEL);
	if (!ispdev) {
		printk("Failed to allocate camera device\n");
		ret = -ENOMEM;
		goto exit;
	}
	spin_lock_init(&ispdev->slock);
	ispdev->dev = &pdev->dev;

	strlcpy(ispdev->media_dev.model, "INGENIC tx_isp",
		sizeof(ispdev->media_dev.model));
	ispdev->media_dev.link_notify = tx_isp_md_link_notify;
	ispdev->media_dev.dev = ispdev->dev;

	v4l2_dev = &ispdev->v4l2_dev;
	v4l2_dev->mdev = &ispdev->media_dev;
	v4l2_dev->notify = tx_isp_v4l2_dev_notify;
	strlcpy(v4l2_dev->name, "tx_isp", sizeof(v4l2_dev->name));

	ret = v4l2_device_register(ispdev->dev, &ispdev->v4l2_dev);
	if (ret < 0) {
		printk("Failed to register v4l2_device: %d\n", ret);
		goto err_v4l2_dev;
	}

	ret = media_device_register(&ispdev->media_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register media device: %d\n", ret);
		goto err_media_dev;
	}

	ret = tx_isp_request_irq(pdev, ispdev);
	if(ret != ISP_SUCCESS){
		v4l2_err(v4l2_dev, "Failed to request isp's irq\n");
		goto err_req_irq;
	}

	mutex_lock(&ispdev->media_dev.graph_mutex);
	ret = bus_for_each_dev(&platform_bus_type, NULL, ispdev, tx_isp_subdev_match);
	if (ret) {
		mutex_unlock(&ispdev->media_dev.graph_mutex);
		v4l2_err(v4l2_dev, "Failed to register isp's subdev\n");
		goto err_match;
	}
	mutex_unlock(&ispdev->media_dev.graph_mutex);

	ret = register_tx_isp_video_in_device(ispdev->dev->platform_data, &ispdev->v4l2_dev);
	if (ret) {
		v4l2_err(v4l2_dev, "Failed to register isp's sensor\n");
		goto err_vin;
	}
	ret = tx_isp_create_media_link_and_pipeline(ispdev);
	if(ret < 0){
		v4l2_err(v4l2_dev, "Failed to create links of media\n");
		goto err_link;
	}

	ret = v4l2_device_register_subdev_nodes(&ispdev->v4l2_dev);
	if (ret) {
		v4l2_err(v4l2_dev, "Failed to register subdev nodes!\n");
		goto err_nodes;
	}

	platform_set_drvdata(pdev, ispdev);

	/* init v4l2_priority */
	v4l2_prio_init(&ispdev->prio);
	isp_debug_init();
	ispdev->revision = 20150320;
	printk("@@@@ tx-isp-probe ok @@@@@\n");

#if TX_ISP_FAST_START
	ret = start_tx_isp_video_in_device();
	if(ret < 0) {
		printk("start_tx_isp_video_in_device error\n");
		return ret;
	}

#if TX_ISP_NRVBS_EN
	ret = tx_isp_frame_channel_fast_start();
	if(ret < 0) {
		printk("tx_isp_frame_channel_fast_start error\n");
		return ret;
	}
#endif

	// Set day or night
	if(g_is_day >= 0) {
		isp_core_tuning_switch_day_or_night(g_is_day);
	}
#endif

	return 0;
err_req_irq:
err_nodes:
err_link:
err_vin:
	tx_isp_release_subdevs(ispdev);
err_match:
	media_device_unregister(&ispdev->media_dev);
err_media_dev:
	v4l2_device_unregister(&ispdev->v4l2_dev);
err_v4l2_dev:
	kfree(ispdev);
exit:
	return ret;

}

static int __exit tx_isp_remove(struct platform_device *pdev)
{
	tx_isp_device_t* ispdev = platform_get_drvdata(pdev);

	tx_isp_free_irq(ispdev);
	tx_isp_release_subdevs(ispdev);
	platform_set_drvdata(pdev, NULL);
	isp_debug_deinit();

	media_device_unregister(&ispdev->media_dev);
	v4l2_device_unregister(&ispdev->v4l2_dev);
	kfree(ispdev);

	return 0;
}

#ifdef CONFIG_PM
static int tx_isp_suspend(struct device *dev)
{
//	tx_isp_device_t* ispdev = dev_get_drvdata(dev);

//	isp_dev_call(ispdev->isp, suspend, NULL);

	return 0;
}

static int tx_isp_resume(struct device *dev)
{
//	tx_isp_device_t* ispdev = dev_get_drvdata(dev);

//	isp_dev_call(ispdev->isp, resume, NULL);

	return 0;
}

static struct dev_pm_ops tx_isp_pm_ops = {
	.suspend = tx_isp_suspend,
	.resume = tx_isp_resume,
};
#endif

static struct platform_driver tx_isp_driver = {
	.probe = tx_isp_probe,
	.remove = __exit_p(tx_isp_remove),
	.driver = {
		.name = "tx-isp",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &tx_isp_pm_ops,
#endif
	},
};

unsigned long ir_switch_mode = 2;
EXPORT_SYMBOL(ir_switch_mode);

unsigned long ir_threshold_min = 2000;
EXPORT_SYMBOL(ir_threshold_min);

unsigned long ir_threshold_max = 2500;
EXPORT_SYMBOL(ir_threshold_max);

unsigned long frame_channel_width = 0;
EXPORT_SYMBOL(frame_channel_width);

unsigned long frame_channel_height = 0;
EXPORT_SYMBOL(frame_channel_height);

unsigned long frame_channel_nrvbs = 0;
EXPORT_SYMBOL(frame_channel_nrvbs);

unsigned long g_camera_lens = 0;
EXPORT_SYMBOL(g_camera_lens);

long g_day_ae_val = 0;
EXPORT_SYMBOL(g_day_ae_val);

long g_night_ae_val = 0;
EXPORT_SYMBOL(g_night_ae_val);

long g_ae_coeff = 0;
EXPORT_SYMBOL(g_ae_coeff);

long g_wb_r = 0;
EXPORT_SYMBOL(g_wb_r);

long g_wb_b = 0;
EXPORT_SYMBOL(g_wb_b);

static int __init ir_switch_parse(char *str)
{
	char *p = NULL;

	p = strstr(str, "ir_mode=");
	if(p != NULL) {
		if(strncmp((p+strlen("ir_mode=")), "off", 3) == 0) {
			ir_switch_mode = 0;
		} else if(strncmp((p+strlen("ir_mode=")), "on", 2) == 0) {
			ir_switch_mode = 1;
		} else if(strncmp((p+strlen("ir_mode=")), "auto", 4) == 0) {
			ir_switch_mode = 2;
		} else {
			printk("Invalid ir_mode info\n");
		}
	}

	p = strstr(str, "min=");
	if(p != NULL) {
		ir_threshold_min = simple_strtoul(p, NULL, 10);
	}

	p = strstr(str, "max=");
	if(p != NULL) {
		ir_threshold_max = simple_strtoul(p, NULL, 10);
	}

	printk("%s mode: %lu threshold min:%lu max:%lu\n", __func__, ir_switch_mode, ir_threshold_min, ir_threshold_max);

	p = strstr(str, "init_vw=");
	if(p != NULL) {
		frame_channel_width = simple_strtoul(p+strlen("init_vw="), NULL, 10);
	}

	p = strstr(str, "init_vh=");
	if(p != NULL) {
		frame_channel_height = simple_strtoul(p+strlen("init_vh="), NULL, 10);
	}

	p = strstr(str, "nrvbs=");
	if(p != NULL) {
		frame_channel_nrvbs = simple_strtoul(p+strlen("nrvbs="), NULL, 10);
	}

	printk("%s width:%lu height:%lu nrvbs:%lu\n", __func__, frame_channel_width, frame_channel_height, frame_channel_nrvbs);

	p = strstr(str, "lens=");
	if(p != NULL) {
		g_camera_lens = simple_strtoul(p+strlen("lens="), NULL, 10);
	}

	printk("%s lens:%lu\n", __func__, g_camera_lens);

	p = strstr(str, "dayEV=");
	if(p != NULL) {
		g_day_ae_val = simple_strtol(p+strlen("dayEV="), NULL, 10);
	}

	p = strstr(str, "nightEV=");
	if(p != NULL) {
		g_night_ae_val = simple_strtol(p+strlen("nightEV="), NULL, 10);
	}

	p = strstr(str, "coeff=");
	if(p != NULL) {
		g_ae_coeff = simple_strtol(p+strlen("coeff="), NULL, 10);
	}

	p = strstr(str, "wbr=");
	if(p != NULL) {
		g_wb_r = simple_strtol(p+strlen("wbr="), NULL, 10);
	}

	p = strstr(str, "wbb=");
	if(p != NULL) {
		g_wb_b = simple_strtol(p+strlen("wbb="), NULL, 10);
	}

	printk("%s dayEv:%ld nightEv:%ld coeff:%ld wbr:%ld wbb:%ld\n", __func__, g_day_ae_val, g_night_ae_val, g_ae_coeff, g_wb_r, g_wb_b);

	return 1;
}
__setup("senv", ir_switch_parse);

static int __init tx_isp_init(void)
{
	return platform_driver_register(&tx_isp_driver);
}

static void __exit tx_isp_exit(void)
{
	platform_driver_unregister(&tx_isp_driver);
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(tx_isp_init);
#else
module_init(tx_isp_init);
#endif
module_exit(tx_isp_exit);

MODULE_AUTHOR("Ingenic xhshen");
MODULE_DESCRIPTION("tx isp driver");
MODULE_LICENSE("GPL");
