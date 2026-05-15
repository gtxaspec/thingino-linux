#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>

#include "include/osd/imp_common.h"
#include "include/osd/imp_isp.h"
#include "include/osd/imp_osd.h"
#include "include/osd/isp_osd.h"
#include "include/osd/osd_mem.h"
#include "include/tx-isp-common.h"

#include "include/user/osd.h"


#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/capability.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/security.h>
#include <linux/export.h>
#include <linux/uaccess.h>
#include <linux/writeback.h>
#include <linux/buffer_head.h>
#include <linux/falloc.h>
#include <asm/ioctls.h>

extern uint8_t logodata_100x100_bgra[40000];

typedef struct IMPISPOSDNode{
	int			  sensorNum;		/*sensor号，单摄时该值为0，详见IMPVI_NUM结构*/
	int			  rAttrIndex;		/**< handle对应的索引*/
	int			  refcnt;			/**< 节点创建的计数，创建时+1，销毁时-1，初始化为0*/
	uint8_t		  oldshowflg;		/*内部使用，记录上一次设置ISPOSD的osd显示状态*/
	IMPIspOsdRngStat status;
	IMPISPOSDNode *prev;
	IMPISPOSDNode *next;
	IMPIspOsdAttrAsm stIspOsdAsm;	/**< 图片属性*/
}IMPISPOSDNode;

typedef struct {
	IMPISPOSDNode	*head;
	IMPISPOSDNode	*tail;
} IMPIspOsdRgnList;

typedef struct {
	//sem_t					semIspRgn;
	IMPIspOsdRgnList		freerlist;
	IMPIspOsdRgnList		busyrlist;
	IMPISPOSDNode			IspOsdNode[MAXISPOSDPIC];/**< ISP绘制OSD的相关属性*/
}IMPISPOSDChn;

typedef struct {
	sem_t					semIspRgn;
	IMPISPOSDChn			IspOsdChn[MAXSUPCHNMUN];
	mem_alloc_info_t*		isp_osd_mem;			 /**ISPOSD使用的内存管理*/
}IMPISPOSD;


IMPISPOSD *gpIspOsd;

#define ISPOSDMAX(a, b) (a > b ? a : b)
#define ISPOSDMIN(a, b) (a < b ? a : b)

/* ISP模块支持的最大摄像头数量 */
#define ISP_MAX_NUM 4
/* 平台支持的最大Sensor数量 */
#define SENSOR_MAX_NUM 4


/******************************************/
IMPISPOSD *getIspOsd()
{
	return gpIspOsd;
}

static void setIspOsd(IMPISPOSD *osd)
{
	gpIspOsd = osd;
	return ;
}

int isp_osd_init(void)
{
	int i = 0,j = 0;
	IMPISPOSD *pIsosd = NULL;
	pIsosd = kmalloc(sizeof(IMPISPOSD), GFP_KERNEL);
	memset(pIsosd,0x0,sizeof(IMPISPOSD));

	/*一共两个通道，每个通道最多8个区域块*/
	for(i = 0;i < MAXSUPCHNMUN;i++){
		for (j = 0; j < MAXISPOSDPIC; j++) {
			pIsosd->IspOsdChn[i].IspOsdNode[j].rAttrIndex = j;
			if (j == 0) {
				pIsosd->IspOsdChn[i].freerlist.head = pIsosd->IspOsdChn[i].freerlist.tail = &pIsosd->IspOsdChn[i].IspOsdNode[0];
				pIsosd->IspOsdChn[i].freerlist.tail->prev = NULL;
				pIsosd->IspOsdChn[i].freerlist.tail->next = NULL;
			} else {
				pIsosd->IspOsdChn[i].freerlist.tail->next = &pIsosd->IspOsdChn[i].IspOsdNode[j];
				pIsosd->IspOsdChn[i].freerlist.tail->next->prev = pIsosd->IspOsdChn[i].freerlist.tail;
				pIsosd->IspOsdChn[i].freerlist.tail = pIsosd->IspOsdChn[i].freerlist.tail->next;
				pIsosd->IspOsdChn[i].freerlist.tail->next = NULL;
			}
		}
	}

	setIspOsd(pIsosd);
	return 0;
}

void IMP_OSD_Exit_ISP(void)
{
	IMPISPOSD *pIsposd = getIspOsd();
	if(NULL == pIsposd) {
		printk("[%s][%d], pIsposd is null,resource has been destroyed\n", __func__,__LINE__);
		return;
	}
	kfree(pIsposd);
	setIspOsd(NULL);
	return ;
}

int isp_osd_create_rgn(int chn,IMPIspOsdAttrAsm *pIspOsdAttr)
{
	if(chn < 0 || chn > 12) {
		printk("[%s][%d] isp_osd_create_rgn chn%d is invalid\n", __func__, __LINE__, chn);
		return -1;
	}
	if(NULL != pIspOsdAttr) {
		printk("[%s][%d]pIspOsdAttr is not null,not support yet.\n", __func__, __LINE__);
		return -1;
	}

	IMPISPOSDNode *pIspOsdNode = NULL;
	IMPISPOSD *pIsposd = getIspOsd();
	if(NULL == pIsposd) {
		printk("[%s][%d], pIsposd is null,resource has been destroyed\n", __func__,__LINE__);
		return -1;
	}

	if (pIsposd->IspOsdChn[chn].freerlist.head == NULL) {
		printk("pIsposd->IspOsdChn[chn].freerlist.head is empty,chn %d only support %d pic\n",chn,MAXISPOSDPIC);
		return -1;
	}

	pIspOsdNode = pIsposd->IspOsdChn[chn].freerlist.head;
	pIsposd->IspOsdChn[chn].freerlist.head = pIsposd->IspOsdChn[chn].freerlist.head->next;

	if (pIsposd->IspOsdChn[chn].freerlist.head == NULL) {
		pIsposd->IspOsdChn[chn].freerlist.tail = NULL;
	} else {
		pIsposd->IspOsdChn[chn].freerlist.head->prev = NULL;
	}

	if (pIsposd->IspOsdChn[chn].busyrlist.head == NULL) {
		pIsposd->IspOsdChn[chn].busyrlist.head = pIsposd->IspOsdChn[chn].busyrlist.tail = pIspOsdNode;
		pIsposd->IspOsdChn[chn].busyrlist.tail->prev = NULL;
		pIsposd->IspOsdChn[chn].busyrlist.tail->next = NULL;
	} else {
		pIsposd->IspOsdChn[chn].busyrlist.tail->next = pIspOsdNode;
		pIsposd->IspOsdChn[chn].busyrlist.tail->next->prev = pIsposd->IspOsdChn[chn].busyrlist.tail;
		pIsposd->IspOsdChn[chn].busyrlist.tail = pIsposd->IspOsdChn[chn].busyrlist.tail->next;
		pIsposd->IspOsdChn[chn].busyrlist.tail->next = NULL;
	}

	pIspOsdNode->status = IMP_ISP_OSD_RGN_BUSY;
	pIspOsdNode->refcnt += 1;
	pIspOsdNode->stIspOsdAsm.stsinglepicAttr.pic.pinum = 9;

	return pIspOsdNode->rAttrIndex;
}


#if 0
void IMP_ISP_Tuning_DumpMask(IMP_ISP_COLOR_VALUE *value)
{
	uint8_t tmp[3];

	memcpy(tmp, &(value->argb), sizeof(tmp));
	/* IMP_LOG_DBG(TAG, "Red:%d\tGreen:%d\tBlue:%d\n", tmp[0], tmp[1], tmp[2]); */
	/* printf("Red:%d\tGreen:%d\tBlue:%d\n", tmp[0], tmp[1], tmp[2]); */
	/* ITU-BT.601-7 */
	value->ayuv.y_value =  0.299    * tmp[0] + 0.587    * tmp[1] + 0.114    * tmp[2];
	value->ayuv.u_value = -0.168736 * tmp[0] - 0.331264 * tmp[1] + 0.500    * tmp[2] + 128;
	value->ayuv.v_value =  0.500    * tmp[0] - 0.418688 * tmp[1] - 0.081312 * tmp[2] + 128;
	/* ITU-BT.709-5 */
	//value->ayuv.y_value =  0.2126   * tmp[0] + 0.7152   * tmp[1] + 0.0722   * tmp[2];
	//value->ayuv.v_value = -0.114572 * tmp[0] - 0.385428 * tmp[1] + 0.5      * tmp[2] + 128;
	//value->ayuv.u_value =  0.5      * tmp[0] - 0.454153 * tmp[1] - 0.045847 * tmp[2] + 128;
	/* IMP_LOG_DBG(TAG, "Y:%d\tU:%d\tV:%d\n", value->ayuv.y_value, */
	/* 			value->ayuv.u_value, value->ayuv.v_value); */
}
#endif
int32_t IMP_ISP_Tuning_SetDrawBlock(IMPVI_NUM num, IMPISPDrawBlockAttr *attr)
{
	struct isp_image_tuning_default_ctrl def_ctrl;
	int32_t ret = 0;
	def_ctrl.vinum = num;
	def_ctrl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	def_ctrl.control.id = IMAGE_TUNING_CID_DRAW_BLOCK_ATTR;
	def_ctrl.control.value = (int)attr;
	isp_core_tunning_unlocked_ioctl_fast(def_ctrl.vinum, def_ctrl.dir, IMAGE_TUNING_CID_DRAW_BLOCK_ATTR, def_ctrl.control.value);
	if(0 != ret) {
		printk("MSG: ioctl IMAGE_TUNING_CID_DRAW_ATTR!\n");
	}

	return ret;
}
int32_t IMP_ISP_Tuning_SetMaskBlock(IMPVI_NUM num, IMPISPMaskBlockAttr *mask)
{
	struct isp_image_tuning_default_ctrl def_ctrl;
	int32_t ret = 0;
	def_ctrl.vinum = num;
	def_ctrl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	def_ctrl.control.id = IMAGE_TUNING_CID_MASK_BLOCK_ATTR;
	def_ctrl.control.value = (uint32_t)mask;
	isp_core_tunning_unlocked_ioctl_fast(def_ctrl.vinum, def_ctrl.dir, IMAGE_TUNING_CID_MASK_BLOCK_ATTR, def_ctrl.control.value);
	if(0 != ret) {
		printk("MSG: ioctl IMAGE_TUNING_CID_MASK_BLOCK_ATTR!\n");
	}

	return ret;
}


int IMP_OSD_SetRgnAttr_ISP(int sensornum,IMPOSDRgnAttr *prAttr,int bosdshow)
{
	int ret = 0;
	IMPISPDrawBlockAttr *pstDrawAttr = NULL;
	IMPISPOSDSingleAttr  *pstOsdAttr = NULL;
	IMPISPMaskBlockAttr *pmask = NULL;
	if(sensornum < 0 || sensornum > 1) {
		printk("%s, Invalidate sensornum,ISP draw osd only support chn0 chn1\n", __func__);
		return IMP_ERR_OSD_CHNID;
	}
	if(NULL == prAttr) {
		printk("%s, Invalidate prAttr\n", __func__);
		return IMP_ERR_OSD_NULL_PTR;
	}
	if(prAttr->type != OSD_REG_ISP_PIC && prAttr->type != OSD_REG_ISP_LINE_RECT && prAttr->type != OSD_REG_ISP_COVER) {
		printk("%s, prAttr->type:%d != (OSD_REG_ISP_PIC/OSD_REG_ISP_LINE_RECT/OSD_REG_ISP_COVER)\n", __func__,prAttr->type);
		return IMP_ERR_OSD_NOT_SUPPORT;
	}

	pstDrawAttr = &prAttr->osdispdraw.stDrawAttr;
	pstOsdAttr = &prAttr->osdispdraw.stpicAttr;
	pmask = &prAttr->osdispdraw.stCoverAttr;
	/*该接口只支持2种ISP绘制OSD类型，一种叠加线、框，一种矩形遮挡*/
	if(prAttr->type == OSD_REG_ISP_LINE_RECT) {
		/*ISPOSD绘制线框类型*/
		ret = IMP_ISP_Tuning_SetDrawBlock(sensornum, pstDrawAttr);
		if(ret < 0){
			printk("%s, IMP_ISP_Tuning_SetDrawBlock error,check param\n", __func__);
			return IMP_ERR_OSD_NOT_SUPPORT;
		}
	} else if(prAttr->type == OSD_REG_ISP_PIC) {
		/*ISPOSD绘制图片类型*/
		//!!!!!注意：现在ISP设置图片类型的都不使用这个接口，后续等ISP完成相关功能陆续使用创建handle号的形式
		printk("%s, Now, OSD_REG_ISP_PIC not use this interface! use isp_osd_create_rgn \n", __func__);
		return IMP_ERR_OSD_NOT_SUPPORT;
	} else if (prAttr->type == OSD_REG_ISP_COVER) {
		/*ISPOSD 绘制矩形遮挡类型*/
		ret = IMP_ISP_Tuning_SetMaskBlock(sensornum, pmask);
		if(ret < 0){
			printk("%s, IMP_ISP_Tuning_SetMaskBlock error,check param\n", __func__);
			return -1;
		}
	}
	return 0;
}

void ISPOSDDraw(IMPOsdRgnType type)
{
	int ret = 0;
	IMPOSDRgnAttr rIspOsdAttr;
	memset(&rIspOsdAttr, 0, sizeof(IMPOSDRgnAttr));

	if (OSD_REG_ISP_LINE_RECT == type) {
		rIspOsdAttr.type = OSD_REG_ISP_LINE_RECT;
		rIspOsdAttr.osdispdraw.stDrawAttr.pinum = 0;
		rIspOsdAttr.osdispdraw.stDrawAttr.type = IMP_ISP_DRAW_LINE;
		rIspOsdAttr.osdispdraw.stDrawAttr.color_type = IMPISP_MASK_TYPE_YUV;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.enable = 1;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.startx = 200; /* Draw vertical lines */
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.starty = 200;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.endx = 800;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.endy = 200;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.color.ayuv.y_value = 255;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.color.ayuv.u_value = 0;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.color.ayuv.v_value = 0;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.width = 5;
		rIspOsdAttr.osdispdraw.stDrawAttr.cfg.line.alpha = 4; /* The range is [0,4], and the smaller the value, the more transparent it becomes */
		ret = IMP_OSD_SetRgnAttr_ISP(0, &rIspOsdAttr, 0);
		if (ret < 0) {
			printk("IMP_OSD_SetRgnAttr_ISP failed\n");
		}
	}

	if (OSD_REG_ISP_COVER == type) {
		rIspOsdAttr.type = OSD_REG_ISP_COVER;
		rIspOsdAttr.osdispdraw.stCoverAttr.chx = 0;
		rIspOsdAttr.osdispdraw.stCoverAttr.pinum = 0;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_en = 1;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_pos_top	= 600;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_pos_left = 600;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_width = 300;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_height = 300;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_type = IMPISP_MASK_TYPE_RGB;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_value.argb.r_value = 0;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_value.argb.g_value = 0;
		rIspOsdAttr.osdispdraw.stCoverAttr.mask_value.argb.b_value = 255;

		ret = IMP_OSD_SetRgnAttr_ISP(0, &rIspOsdAttr, 0);
		if (ret < 0) {
			printk("IMP_OSD_SetRgnAttr_ISP failed\n");
		}
	}

	return;
}

int IspOsd_rgnsize(IMPIspOsdAttrAsm *pIspOsdNode)
{
	int size = 0;
	int stride = 0,height = 0;
	if (NULL == pIspOsdNode) {
		printk("[%s][%d] pIspOsdNode is null,check param!\n", __func__, __LINE__);
		return -1;
	}
	stride = pIspOsdNode->stsinglepicAttr.pic.osd_stride;//200*4
	height = pIspOsdNode->stsinglepicAttr.pic.osd_height;//200
	if(stride <= 0 || height <= 0) {
		printk("[%s][%d] stride(%d) height(%d) is invalid,check!\n", __func__, __LINE__,stride,height);
		return -1;
	}
	size = stride*height;
	//printk("rgn size:%d,height:%d,stride:%d\n",size ,pIspOsdNode->stsinglepicAttr.pic.osd_height,pIspOsdNode->stsinglepicAttr.pic.osd_stride);
	return size;
}


int32_t IMP_ISP_Tuning_SetOSDAttr(IMPVI_NUM num, IMPISPOSDAttr *attr)
{
	struct isp_image_tuning_default_ctrl def_ctrl;
	int32_t ret = 0;
	def_ctrl.vinum = num;
	def_ctrl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	def_ctrl.control.id = IMAGE_TUNING_CID_OSD_ATTR;
	def_ctrl.control.value = (int)attr;
	isp_core_tunning_unlocked_ioctl_fast(def_ctrl.vinum, def_ctrl.dir, IMAGE_TUNING_CID_OSD_ATTR, def_ctrl.control.value);
	if(0 != ret) {
		printk("MSG: ioctl IMAGE_TUNING_CID_OSD_ATTR!\n");
	}

	return ret;
}


void OsdnotifyIsp(IMPISPOSDNode * pIspOsdhnode)
{
	int ret = 0;
	IMPISPOSDNode * pnode = pIspOsdhnode;
	void *vaddr = NULL,*paddr = NULL;
	if(pnode->rAttrIndex == pnode->stIspOsdAsm.stsinglepicAttr.pic.pinum)
	{
		//printk("pnode->rAttrIndex   = %d, pnode->stIspOsdAsm.stsinglepicAttr.pic.pinum = %d\n", pnode->rAttrIndex , pnode->stIspOsdAsm.stsinglepicAttr.pic.pinum);
		vaddr = pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_image;
		paddr = virt_to_phys(vaddr);
		pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_image = paddr;//ISP需要使用物理地址，因此转化

		if(((uint32_t)paddr &(~0x7)) != (uint32_t)paddr){
			printk("[%s][%d]##################pData:%p is not 8 align,handl%d####################\n",__func__,__LINE__,paddr,pnode->rAttrIndex);
		}
		if((uint32_t)paddr < 0x100000){
			printk("[%s][%d]@@@@@@@@@@@@@@@@@@pData:%p addr is < 1M @@@@@@@@@@@@@@@@@@@@@@@@@@\n",__func__,__LINE__,paddr);
		}

		ret = IMP_ISP_Tuning_SetOSDAttr((IMPVI_NUM)pnode->sensorNum, &pnode->stIspOsdAsm.stsinglepicAttr.chnOSDAttr);
		if(ret < 0) {
			printk("[%s][%d]	IMP_ISP_Tuning_SetOSDAttr err!!!\n", __func__, __LINE__);
			return ;
		}

		ret = IMP_ISP_Tuning_SetOSDBlock((IMPVI_NUM)pnode->sensorNum, &pnode->stIspOsdAsm.stsinglepicAttr.pic);
		if(ret < 0) {
			printk("[%s][%d]	IMP_ISP_Tuning_SetOSDBlock err!!!\n", __func__, __LINE__);
			return ;
		}
		pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_image = vaddr;
		//printk("pnode->rAttrIndex   = %d, pnode->stIspOsdAsm.stsinglepicAttr.pic.pinum = %d\n", pnode->rAttrIndex , pnode->stIspOsdAsm.stsinglepicAttr.pic.pinum);
	}

	return;
}

int32_t IMP_ISP_Tuning_SetOSDBlock(IMPVI_NUM num, IMPISPOSDBlockAttr *attr)
{
	struct isp_image_tuning_default_ctrl def_ctrl;
	int32_t ret = 0;
	def_ctrl.vinum = num;
	def_ctrl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	def_ctrl.control.id = IMAGE_TUNING_CID_OSD_BLOCK_ATTR;
	def_ctrl.control.value = (int)attr;
	isp_core_tunning_unlocked_ioctl_fast(def_ctrl.vinum, def_ctrl.dir, IMAGE_TUNING_CID_OSD_BLOCK_ATTR, def_ctrl.control.value);
	if(0 != ret) {
		printk("MSG: ioctl IMAGE_TUNING_CID_OSD_BLOCK_ATTR!\n");
	}

	return ret;
}

int iscrossrect(uint16_t minx1,uint16_t miny1,uint16_t maxx1,uint16_t maxy1,uint16_t minx2,uint16_t miny2,uint16_t maxx2,uint16_t maxy2)
{
	uint16_t minx = 0,miny = 0,maxx = 0,maxy = 0;
	minx = ISPOSDMAX(minx1,minx2);
	miny = ISPOSDMAX(miny1,miny2);
	maxx = ISPOSDMIN(maxx1,maxx2);
	maxy = ISPOSDMIN(maxy1,maxy2);
	if(minx < maxx && miny < maxy){
		//相交返回1
		return 1;
	}
	return 0;
}

void IspOsdAdjust_Pic(int chn,IMPIspOsdRgnList *pIspRgnList)
{
	if(NULL == pIspRgnList) {
		printk("[%s][%d]pIspRgnList is null,check!\n", __func__, __LINE__);
		return;
	}
	IMPISPOSDNode *phead = NULL,*pnode = NULL;
	uint16_t srcw = 0,srch = 0,src_startx = 0,src_starty = 0, src_pinum = 0;
	uint16_t dstw = 0,dsth = 0,dst_startx = 0,dst_starty = 0, dst_pinum = 0;
	uint16_t src_overflag;
	uint16_t dst_overflag;
	phead = pIspRgnList->head;
	while(phead) {
		if(phead->stIspOsdAsm.stsinglepicAttr.pic.osd_enable) {
			srcw = phead->stIspOsdAsm.stsinglepicAttr.pic.osd_width;
			srch = phead->stIspOsdAsm.stsinglepicAttr.pic.osd_height;
			src_startx = phead->stIspOsdAsm.stsinglepicAttr.pic.osd_left;
			src_starty = phead->stIspOsdAsm.stsinglepicAttr.pic.osd_top;
			src_pinum = phead->stIspOsdAsm.stsinglepicAttr.pic.pinum;
			if(0 == src_pinum || 1 == src_pinum || 2 == src_pinum || 3 == src_pinum) {
				src_overflag = 0;
			} else {
				src_overflag = 1;
			}
		}
		pnode = phead->next;
		while(pnode) {
			if(pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_enable) {
				dstw = pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_width;
				dsth = pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_height;
				dst_startx = pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_left;
				dst_starty = pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_top;
				dst_pinum = phead->stIspOsdAsm.stsinglepicAttr.pic.pinum;
				if(0 == dst_pinum || 1 == dst_pinum || 2 == dst_pinum || 3 == dst_pinum) {
					dst_overflag = 0;
				} else {
					dst_overflag = 1;
				}
				if((src_overflag == dst_overflag) && (src_pinum != dst_pinum)) {
					/*判断若相交，则链表靠后的节点对应的使能开关关闭，否则会产生乱码问题*/
					if(iscrossrect(src_startx,src_starty,src_startx+srcw,src_starty+srch,dst_startx,dst_starty,dst_startx+dstw,dst_starty+dsth))
					{
						printk("src_pinum = %d, dst_pinum = %d, src_overflag = %d, dst_overflag = %d\n", src_pinum, dst_pinum, src_overflag, dst_overflag);
						printk("hand%d has changed showflg,positon(%d,%d)(w,h)(%d,%d)\n",
								pnode->rAttrIndex,
								pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_left,
								pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_top,
								pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_width,
								pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_height);
						pnode->stIspOsdAsm.stsinglepicAttr.pic.osd_enable = 0;//若相交，则关闭相交的块
						pnode->oldshowflg = 0;
					}
				}
			}
			pnode = pnode->next;
		}
		phead = phead->next;

	}
	return ;
}

//判断busy
//
int IMP_OSD_SetRgnAttr_PicISP(int chn,int handle, IMPIspOsdAttrAsm *pIspOsdAttr)
{
	int newrectsize = 0,oldrectsize = 0;
	if (handle >= MAXISPOSDPIC || handle < 0) {
		printk("[%s][%d] ISPOSD Invalidate handle(%d)\n", __func__,__LINE__,handle);
		return -1;
	}
	if (chn < 0 || chn > 1) {
		printk("[%s][%d]	chn%d is invalid\n", __func__, __LINE__, chn);
		return -1;
	}
	if (NULL == pIspOsdAttr) {
		printk("[%s][%d] prAttr Invalidate param\n", __func__,__LINE__);
		return -1;
	}

	int ret = 0;
	uint32_t width = 0, height = 0;
	uint16_t top = 0, left = 0, osdw = 0, osdh = 0;
	width  = 1920;//sensor分辨率
	height = 1080;
	left = pIspOsdAttr->stsinglepicAttr.pic.osd_left;
	top  = pIspOsdAttr->stsinglepicAttr.pic.osd_top;
	osdw = pIspOsdAttr->stsinglepicAttr.pic.osd_width;
	osdh = pIspOsdAttr->stsinglepicAttr.pic.osd_height;
	if((top + osdh) > height || (left + osdw > width) ) {
		printk("[%s][%d]	WRNING!!!(top:%d+osdh:%d)=(%d) (left:%d+osdw:%d)=(%d) frame(w:%d,h:%d)\n", __func__, __LINE__,\
			top, osdh, (top+osdh), left, osdw, (left+osdw), width, height);
		return -1;
	}

	void *pData = NULL;
	IMPIspOsdAttrAsm *pOriIspOsdAttr = NULL;
	IMPISPOSD *pIsposd = getIspOsd();
	if(NULL == pIsposd){
		printk("[%s][%d], pIsposd is null,resource has been destroyed\n", __func__,__LINE__);
		return -1;
	}

	if (pIsposd->IspOsdChn[chn].IspOsdNode[handle].status == IMP_ISP_OSD_RGN_FREE) {
		printk("%s, the chn%d region %d hasn't been created\n", __func__,chn,handle);
		return -1;
	}

	pOriIspOsdAttr = &pIsposd->IspOsdChn[chn].IspOsdNode[handle].stIspOsdAsm;
	oldrectsize = pOriIspOsdAttr->stsinglepicAttr.pic.osd_stride*pOriIspOsdAttr->stsinglepicAttr.pic.osd_height;
	if((newrectsize = IspOsd_rgnsize(pIspOsdAttr)) <= 0){
		printk("%s,%d:malloc pData size=%d failed\n", __func__, __LINE__, newrectsize);
		return -1;
	}

	if(ISP_OSD_REG_PIC == pIspOsdAttr->type) {
		if ((oldrectsize != newrectsize)&&(0 == oldrectsize)) {
			pData = kmalloc(newrectsize, GFP_KERNEL);
			memset(pData, 0, newrectsize);
		} else if (oldrectsize == newrectsize) {
			pData = pOriIspOsdAttr->stsinglepicAttr.pic.osd_image;
		} else {
			printk("%s, the chn%d region %d not support changed pic size dynamic,should destroy rgn and recreate\n", __func__,chn,handle);
			return -1;
		}
	} else {
		printk("%s, the chn%d region %d OSD type(%d) isn't ISP_OSD_REG_PIC\n", __func__,chn,handle,pIspOsdAttr->type);
		return -1;
	}

	memcpy(pOriIspOsdAttr, pIspOsdAttr, sizeof(IMPIspOsdAttrAsm));
	if(pIspOsdAttr->stsinglepicAttr.pic.osd_image){
		memcpy(pData,pIspOsdAttr->stsinglepicAttr.pic.osd_image,newrectsize);
	} else {
		memset(pData,0,newrectsize);
	}

	pOriIspOsdAttr->stsinglepicAttr.pic.osd_enable = 0;//设置后不生效，需要再显示调用show接口(show接口中存在相交判定操作)
	pOriIspOsdAttr->stsinglepicAttr.pic.osd_image = pData;

	return 0;
}
int IMP_OSD_ShowRgn_ISP( int chn,int handle, int showFlag)
{
	if (handle >= MAXISPOSDPIC || handle < 0) {
		printk("[%s][%d] ISPOSD Invalidate handle(%d)\n", __func__,__LINE__,handle);
		return -1;
	}
	if (chn < 0 || chn > 12) {
		printk("[%s][%d]	chn%d is invalid\n", __func__, __LINE__, chn);
		return -1;
	}

	IMPISPOSDNode *pIspOsdNode = NULL;
	IMPISPOSD *pIsposd = getIspOsd();
	if(NULL == pIsposd) {
		printk("[%s][%d], pIsposd is null,resource has been destroyed\n", __func__,__LINE__);
		return -1;
	}

	pIspOsdNode = &pIsposd->IspOsdChn[chn].IspOsdNode[handle];

	if (pIspOsdNode->status == IMP_ISP_OSD_RGN_FREE) {
		printk("[%s][%d] chn%d rgn is free,can't set showflag\n", __func__, __LINE__, chn);
		return -1;
	}

	pIspOsdNode->stIspOsdAsm.stsinglepicAttr.pic.osd_enable = showFlag;/*虽然这里设置了，但ISP不知道，还必须要再设置一次*/
	pIspOsdNode->oldshowflg = showFlag;
	pIspOsdNode->sensorNum = chn;

	/*调整位置：因为ISP硬件资源对重叠区域没有做好处理，导致相交、覆盖的osd出现乱码，断流，因此需要软件做出处理，避免断流情况发生*/
	IspOsdAdjust_Pic(chn,&pIsposd->IspOsdChn[chn].busyrlist);
	OsdnotifyIsp(pIspOsdNode);

	return 0;
}

void draw_pic(int pic_handle)
{
	int ret = 0;
	IMPIspOsdAttrAsm stISPOSDAsm;
	stISPOSDAsm.type = ISP_OSD_REG_PIC;
	stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_type = IMP_ISP_PIC_ARGB_8888;
	stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_group = 0;
	stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_argb_type = IMP_ISP_ARGB_TYPE_BGRA;
	stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_pixel_alpha_disable = IMPISP_TUNING_OPS_MODE_DISABLE;
	stISPOSDAsm.stsinglepicAttr.pic.pinum = pic_handle;//你通过IMP_ISP_Tuning_CreateOsdRgn得到的handle值
	stISPOSDAsm.stsinglepicAttr.pic.osd_enable = 1;
	stISPOSDAsm.stsinglepicAttr.pic.osd_left = 100;
	stISPOSDAsm.stsinglepicAttr.pic.osd_top = 100;
	stISPOSDAsm.stsinglepicAttr.pic.osd_width = 100;
	stISPOSDAsm.stsinglepicAttr.pic.osd_height = 100;
	stISPOSDAsm.stsinglepicAttr.pic.osd_image = logodata_100x100_bgra;//你的图片的位置
	stISPOSDAsm.stsinglepicAttr.pic.osd_stride = 100*4;

	ret = IMP_OSD_SetRgnAttr_PicISP(0, pic_handle, &stISPOSDAsm);
	if (ret < 0) {
		printk("IMP_ISP_Tuning_SetOsdRgnAttr failed\n");
		return;
	}

	ret = IMP_OSD_ShowRgn_ISP(0, pic_handle, 1);
	if (ret < 0) {
		printk("IMP_ISP_Tuning_ShowOsdRgn failed\n");
		return;
	}
}

int isp_osd_thread_create(IMP_Sample_OsdParam *isposdparam)
{
    int err;
	struct task_struct *osd_task;
    isposdparam->osd_task = kthread_run(update_time, isposdparam, "isp_osd_update");
	osd_task = isposdparam->osd_task;
    if(IS_ERR(osd_task)){
        printk("Unable to start kernel thread. ");
        err = PTR_ERR(osd_task);
        osd_task = NULL;
        return err;
    }
    return 0;
}


int ISPOSD(void *arg)
{
	int ret = 0;
	IMP_Sample_OsdParam *isposdparam = (IMP_Sample_OsdParam*)arg;
	/* Draw lines, boxes, and rectangles to obscure */
	ISPOSDDraw(OSD_REG_ISP_LINE_RECT);
	ISPOSDDraw(OSD_REG_ISP_COVER);

	/* Draw an image, paying attention to the difference between the ISP interface for drawing image types and the interface for drawing lines, boxes, and rectangles for occlusion */
	draw_pic(isposdparam->phandles[0]);

	/* Draw timestamp */
	ret = isp_osd_thread_create(isposdparam);
	if ( ret < 0){
		printk("isposd_thread_create err!\n");
	}

	return 0;
}

