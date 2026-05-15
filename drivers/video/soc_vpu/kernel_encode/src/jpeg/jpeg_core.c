#include "jpeg_core.h"
#include "vpu_common.h"
#include "helix.h"

static uint32_t qmem_t32v[QMEM_LEN] = {
	//normal quantization table
	0x0e0c0b10, 0x0e100a0c, 0x11120e0d, 0x28181310, 0x1616181a, 0x25233118, 0x333a281d,
	0x33393c3d, 0x48403738, 0x44404e5c, 0x38374557, 0x57516d50, 0x6867625f, 0x714d3e67,
	0x78647079, 0x6367655c, 0x18121211, 0x1a2f1815, 0x42632f1a, 0x63634238, 0x63636363,
	0x63636363, 0x63636363, 0x63636363, 0x63636363, 0x63636363, 0x63636363, 0x63636363,
	0x63636363, 0x63636363, 0x63636363, 0x63636363
};

int jpeg_param_init(jpeg_t *inst, struct jpeg_param *param)
{
	struct jpeg_instance *jpeg = (struct jpeg_instance *)inst;
	memset(jpeg, 0, sizeof(struct jpeg_instance));
	memcpy(&jpeg->param, param, sizeof(struct jpeg_param));
	return 0;
}

void jpeg_exit(jpeg_t *inst)
{
	return;
}

static int jpeg_set_slice_info(struct jpeg_slice_info *si, struct vpu_io_info *io)
{
	int mb_width = 0;
	int mb_height = 0;
	memset(si, 0, sizeof(struct jpeg_slice_info));
	/* configure Quantization Table */
	memcpy((void *)si->qmem, (void *)qmem_t32v, QMEM_LEN * 4);
	/* 2.configure jpeg ctrl(qt, reset_mode, video_mode) */
	si->jpeg_ctrl |= 1 << CTRL_HD_U | 1 << CTRL_HA_U | 1 << CTRL_HD_V | 1 << CTRL_HA_V |
		1 << CTRL_QT_U | 1 << CTRL_QT_V | 0 << CTRL_QT_Y |
		0 << AWLEN | 0 << LIMIT_BS_LEN_EN | 0 << TIMEOUT_EN |
		io->qp << CTRL_QP | 1 << JPEG_FRAME_END_INTR_EN | 0<<JPEG_FRAME_END_BSLESS_INTR_EN |
		0 << IVDC_STOP_REQ_INTR_EN | 0 << JPEG_TIMEOUT_INTR_EN;

	//3.configure
	/* (1)configure MCU */
	mb_width = C_ALIGN(io->width, 16) / 16;;
	mb_height = C_ALIGN(io->height, 16) / 16;;
	si->nmcu = mb_width * mb_height - 1;

	/* (2)configure width/height/format */
	si->frm_info = (io->width << FRM_WIDTH) | (io->height << FRM_HEIGHT);

	/* (3)configure LINE_STRD */
	si->stride = mb_width * 16;

	/* (4)configure Y/UV base addr */
	si->y_addr = (unsigned int)io->raw_y_phyaddr;
	si->c_addr = (unsigned int)io->raw_c_phyaddr;

	/* (5)configure BS write out base addr */
	si->bs_addr = (unsigned int)io->bs_phyaddr;

	/* (6)configure pixel total number after align(16) */
	si->pixel_num = (unsigned int)(mb_width * 16 * mb_height * 16 * 3 / 2);

	/* (7)configure restart markers */
	//if (s->restart_markers_en) {
	if (0) {
		si->marker_en = 1;
		si->marker_num = MARKERS_NUMBER;
	} else {
		si->marker_en = 0;
		si->marker_num = 0;
	}

	/* (8)configure restart markers */
	si->limit_bs_len = 0;
	return 0;
}

static int jpeg_hw_start(jpeg_t *inst, struct vpu_io_info *io)
{
	struct jpeg_instance *jpeg = (struct jpeg_instance *)inst;
	struct jpeg_slice_info *info = &jpeg->reg_info;
	int timeout = 0x7fff;
	int i = 0;

	//vpu reset
	vpu_wrap_write_reg(JPEG_BASE + JPEG_TRIG, 0x1 << JPEG_SEQ_RESET);
	while (!(vpu_wrap_read_reg(JPEG_BASE + JPEG_TRIG) & (0x1 << RESET_END)) && --timeout) {
		vpu_wrap_usleep(20);
	}
	if (timeout == 0) {
		VPU_ERROR("jpeg reset err!\n");
		return -1;
	}

	//vpu start
	for (i = 0; i < QMEM_LEN; i ++) {
		vpu_wrap_write_reg(JPEG_BASE + JPEG_QT_BASE_TAB + 4 * i, info->qmem[i]);
	}
	vpu_wrap_write_reg(JPEG_BASE + JPEG_CTRL, info->jpeg_ctrl);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_NMCU, info->nmcu);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_FRM_INFO, info->frm_info);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_LINE_STRD, info->stride);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_RAWY_ADDR, info->y_addr);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_RAWC_ADDR, info->c_addr);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_BS_ADDR, info->bs_addr);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_CODE_PIX_TOTAL, info->pixel_num);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_MARKERS, info->marker_en << MARKERS_ENABLE | (info->marker_num - 1) << MARKERS_NUMBER);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_LIMIT_BS_LEN_VAL, info->limit_bs_len);
	vpu_wrap_write_reg(JPEG_BASE + JPEG_TRIG, vpu_wrap_read_reg(JPEG_BASE + JPEG_TRIG) | (1 << JPEG_START));

	//vpu intr
	timeout = 0x7fff;
	while(!(vpu_wrap_read_reg(JPEG_BASE + JPEG_INTR_STA) & (0x1 << JPEG_FRAME_END_INTR)) && --timeout) {
		//printf("RADIX_REG_CFGC_STAT=0x%08x\n", vpu_wrap_read_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_STAT));
		vpu_wrap_usleep(20);
	}
	if (timeout == 0) {
		VPU_ERROR("vpu wait intr timeout!\n");
		return -1;
	}
	vpu_wrap_write_reg(JPEG_BASE + JPEG_INTR_CLR, (vpu_wrap_read_reg(JPEG_BASE + JPEG_INTR_CLR)) | (0x1 << JPEG_FRAME_END_INTR));
	io->bslen = vpu_wrap_read_reg(JPEG_BASE + JPEG_GLB_BS_LEN);
	if (io->vpu_stream_buffer_occupancy + io->bslen > io->vpu_stream_buffer_len) {
		VPU_ERROR("jpeg no free stream buf\n");
		return VPU_BUFFULL;
	}
	memcpy((char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy, (void *)io->bs_viraddr, io->bslen);
	io->vpu_curr_stream_len = io->bslen;
	printk("kernel encode %d curr len = %d\n", io->tot_frame_num, io->vpu_curr_stream_len);
	return 0;
}

int jpeg_encode(jpeg_t *inst, struct vpu_io_info *io)
{
	struct jpeg_instance *jpeg = (struct jpeg_instance *)inst;

	//init reg
	jpeg_set_slice_info(&jpeg->reg_info, io);

	//start hw
	return jpeg_hw_start(jpeg, io);
}
