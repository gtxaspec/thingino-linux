/*
 * Cryptographic API.
 *
 * Support for Ingenic AES HW acceleration.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/crypto.h>
#include <linux/interrupt.h>
#include <crypto/scatterwalk.h>
#include <crypto/aes.h>
#include <linux/delay.h>
#include "ingenic-aes.h"
#include <crypto/internal/skcipher.h>

/* keep registered devices data here */
static LIST_HEAD(dev_list);
static DEFINE_SPINLOCK(list_lock);


static inline unsigned long aes_read(struct ingenic_aes_dev *aes, unsigned long offset)
{
	return readl(aes->io_base + offset);
}

static inline void aes_write(struct ingenic_aes_dev *aes, unsigned long offset,
	unsigned long value)
{
	writel(value, aes->io_base + offset);
}

void dump_aes_reg(struct ingenic_aes_dev *aes){
	dev_info(aes->dev,"AES_ASCR: 0x%08lx\n",aes_read(aes,AES_ASCR));
	dev_info(aes->dev,"AES_ASSR: 0x%08lx\n",aes_read(aes,AES_ASSR));
	dev_info(aes->dev,"AES_ASINTM: 0x%08lx\n",aes_read(aes,AES_ASINTM));
	dev_info(aes->dev,"AES_ASSA: 0x%08lx\n",aes_read(aes,AES_ASSA));
	dev_info(aes->dev,"AES_ASDA: 0x%08lx\n",aes_read(aes,AES_ASDA));
	dev_info(aes->dev,"AES_ASTC: 0x%08lx\n",aes_read(aes,AES_ASTC));
}

static inline int ingenic_aes_hw_init(struct ingenic_aes_dev *aes)
{
	/*
	 * clocks are enabled when request starts and disabled when finished.
	 * It may be long delays between requests.
	 * Device might go to off mode to save power.
	 */
	pm_runtime_get_sync(aes->dev);

	if (!(aes->flags & FLAGS_INIT)) {
		aes->flags |= FLAGS_INIT;
		aes->err = 0;
	}

	return 0;
}

static inline int check_keydone(struct ingenic_aes_dev *aes)
{
	unsigned long timeout = 1000;
	while(!(aes_read(aes,AES_ASSR) & (1 << 0)) && --timeout);
	if(timeout == 0){
		dev_err(aes->dev,"Write keys is timeout.\n");
		return -1;
	}
	return 0;
}

static inline void ingenic_aes_stop(struct ingenic_aes_dev *aes)
{
	unsigned long d;
	d = aes_read(aes, AES_ASCR);
	d &= ~1;
	aes_write(aes, AES_ASCR,d);
	aes->flags = 0;
	pm_runtime_put_sync(aes->dev);
	pr_debug("ingenic aes stop. finished!\n");
}
static inline int ingenic_aes_write_ctrl(struct ingenic_aes_dev *aes)
{
	unsigned int key32;
	int i;
	unsigned long val,init_data,mode,alg;
	int err;
	err = ingenic_aes_hw_init(aes);
	if (err){
		dev_err(aes->dev,"aes hw init failed.\n");
		return err;
	}
	// 1. Mask interrupt and clear interrupt.
	aes_write(aes,AES_ASINTM,0);
	aes_read(aes,AES_ASSR);

    // 2. AES enable and clear data for new process.
	init_data = ASCR_PS_DEF | ASCR_CLR | ASCR_EN;
	aes_write(aes,AES_ASCR,init_data);

	// 3. configure AES

	key32 = aes->ctx->keylen / sizeof(unsigned long);
	switch(aes->ctx->keylen){
	case AES_KEYSIZE_128:
		val = 0;
		break;
	case AES_KEYSIZE_192:
		val = 1;
		break;
	case AES_KEYSIZE_256:
		val = 2;
		break;
	default:
		dev_err(aes->dev,"len error\n");
		return -EINVAL;
	}
	alg = (aes->flags & FLAGS_CBC) == FLAGS_CBC ? CBC_ALG:ECB_ALG;
	mode = (aes->flags & FLAGS_DECRYPT) == FLAGS_DECRYPT ? DECRYPTO_MODE : ENCRYPTO_MODE;
	init_data = ASCR_PS_DEF | ASCR_KEYL(val) | ASCR_ALGS(alg) | ASCR_DECE(mode) | ASCR_EN;
	aes_write(aes,AES_ASCR,init_data);

	if (alg == CBC_ALG){
		// Initialize IV of cbc alg
		unsigned int *dat = (unsigned int *)aes->req->iv;
		if(dat == NULL){
			dev_err(aes->dev,"no set iv data in cbc(aes)\n");
		}
		for (i = 0; i < 4; i++) {
			aes_write(aes, AES_ASIV,__be32_to_cpu(dat[i]));
			// aes_write(aes, AES_ASIV,dat[i]);
			// printk("======== iv:0x%08x 0x%08x\n",dat[i],__be32_to_cpu(dat[i]));
		}
		init_data = aes_read(aes,AES_ASCR);
#ifdef CONFIG_SOC_PRJ008
		if(mode == DECRYPTO_MODE){
			// init_data |= (1<<13 | 1 << 12); // 小端输入输出
		}else {
			init_data &= ~(1 << 12); //大端输入
			init_data |= 1 << 13; // 小端输出
		}
#endif
		aes_write(aes,AES_ASCR,init_data | ASCR_INIT_IV);
	}
	// Initialize Key.
	for (i = 0; i < key32; i++) {
		aes_write(aes, AES_ASKY,
			__be32_to_cpu(aes->ctx->key[i]));
		// printk("=====key=== 0x%08lx 0x%08x\n",aes->ctx->key[i],__be32_to_cpu(aes->ctx->key[i]));
	}

	init_data = aes_read(aes,AES_ASCR);
	aes_write(aes,AES_ASCR,init_data | ASCR_KEYS); // check key done in dma process.
	return 0;
}

static struct ingenic_aes_dev *ingenic_aes_find_dev(struct ingenic_aes_ctx *ctx)
{
	struct ingenic_aes_dev *aes = NULL, *tmp;

	spin_lock(&list_lock);
	if (!ctx->aes) {
		list_for_each_entry(tmp, &dev_list, list) {
			/* FIXME: take fist available aes core */
			aes = tmp;
			break;
		}
		ctx->aes = aes;
	} else {
		/* already found before */
		aes = ctx->aes;
	}
	spin_unlock(&list_lock);

	return aes;
}

static int ingenic_aes_dma_init(struct ingenic_aes_dev *aes)
{
	int err = -ENOMEM;
	aes->buf_in = (void *)__get_free_pages(GFP_KERNEL, INGENIC_AES_CACHE_PAGE_SHIFT);
	aes->buf_out = (void *)__get_free_pages(GFP_KERNEL, INGENIC_AES_CACHE_PAGE_SHIFT);
	aes->buflen = PAGE_SIZE << INGENIC_AES_CACHE_PAGE_SHIFT;
	aes->buflen &= ~(AES_BLOCK_SIZE - 1);

	if (!aes->buf_in || !aes->buf_out) {
		dev_err(aes->dev, "unable to alloc pages.\n");
		goto err_alloc;
	}
	aes->dma_addr_in = dma_map_single(aes->dev, aes->buf_in, aes->buflen,
		DMA_TO_DEVICE);
	if (dma_mapping_error(aes->dev, aes->dma_addr_in)) {
		dev_err(aes->dev, "dma %d bytes error\n", aes->buflen);
		err = -EINVAL;
		goto err_map_in;
	}

	aes->dma_addr_out = dma_map_single(aes->dev, aes->buf_out, aes->buflen,
		DMA_FROM_DEVICE);
	if (dma_mapping_error(aes->dev, aes->dma_addr_out)) {
		dev_err(aes->dev, "dma %d bytes error\n", aes->buflen);
		err = -EINVAL;
		goto err_map_out;
	}
	return 0;
err_map_out:
	dma_unmap_single(aes->dev, aes->dma_addr_in, aes->buflen, DMA_TO_DEVICE);
err_map_in:
	free_pages((unsigned long)aes->buf_out, INGENIC_AES_CACHE_PAGE_SHIFT);
	free_pages((unsigned long)aes->buf_in, INGENIC_AES_CACHE_PAGE_SHIFT);
err_alloc:
	if (err)
		pr_err("error: %d\n", err);
	return err;
}

static void ingenic_aes_dma_cleanup(struct ingenic_aes_dev *aes)
{
	free_pages((unsigned long)aes->buf_out, INGENIC_AES_CACHE_PAGE_SHIFT);
	free_pages((unsigned long)aes->buf_in, INGENIC_AES_CACHE_PAGE_SHIFT);
}

// 辅助函数：获取SG链表中，从起始位置偏移offset字节后的虚拟地址
// 参数：
//   sg: SG链表头
//   sg_offset: SG链表的初始操作偏移（如aes->out_offset）
//   offset: 相对于初始操作偏移的偏移量（如加密长度 - AES_BLOCK_SIZE）
// 返回：成功返回指针，失败返回NULL
// static void *sg_get_virt_ptr(struct scatterlist *sg, size_t sg_offset, size_t offset) {
// 	struct scatterlist *cur_sg = sg;
// 	size_t remaining = sg_offset + offset; // 总偏移：初始偏移 + 目标偏移

// 	// 遍历SG链表，找到包含remaining的节点
// 	while (cur_sg) {
// 		size_t sg_len = sg->length; // 当前SG节点的长度
// 		if (remaining < sg_len) {
// 			// 找到目标节点：返回该节点的虚拟地址 + remaining偏移
// 			return sg_virt(cur_sg) + remaining;
// 		}
// 		// 未找到，减去当前节点长度，继续下一个节点
// 		remaining -= sg_len;
// 		cur_sg = sg_next(cur_sg);
// 	}

// 	// 偏移超出SG链表总长度，返回NULL
// 	pr_err("sg_get_virt_ptr: offset out of range (sg_offset=%zu, offset=%zu)\n", sg_offset, offset);
// 	return NULL;
// }

static void sg_copy_buf(void *buf, struct scatterlist *sg,
	unsigned int start, unsigned int nbytes, int out)
{
	struct scatter_walk walk;

	if (!nbytes)
		return;

	scatterwalk_start(&walk, sg);
	scatterwalk_advance(&walk, start);
	scatterwalk_copychunks(buf, &walk, nbytes, out);
	scatterwalk_done(&walk, out, 0);
}

static int sg_copy(struct scatterlist **sg, size_t *offset, void *buf,
	size_t buflen, size_t total, int out)
{
	unsigned int count, off = 0;

	while (buflen && total) {
		count = min((*sg)->length - *offset, total);
		count = min(count, buflen);

		if (!count)
			return off;

		/*
		 * buflen and total are AES_BLOCK_SIZE size aligned,
		 * so count should be also aligned
		 */

		sg_copy_buf(buf + off, *sg, *offset, count, out);

		off += count;
		buflen -= count;
		*offset += count;
		total -= count;
#if 0
		if((count == 320)&&(out == 1)){
			int i;
			for(i = 0;i < 16;i+=4){
				unsigned int *d = sg_virt(*sg)+ 320 - 16;//;
				unsigned int *k = sg_virt(*sg);
				printk("320sg: %5d:0x%08x    0x%08x   0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]),__be32_to_cpu(k[i/4]));
			}
		}

		if (sg_is_last(*sg) && count < (*sg)->length) {
			if (*sg)
				printk("last sg partial copy, sg len: %d, count: %d\n",(*sg)->length, count);
			// (*sg)->length = count;
		}
#endif
		if (*offset == (*sg)->length) {
			*sg = sg_next(*sg);
			if (*sg)
				*offset = 0;
			else
				total = 0;
		}

	}

	return off;
}

void dump_sgdata(struct scatterlist *sg)
{
	int i;
	unsigned int *d = sg_virt(sg);
	for(i = 0;i < sg->length;i += 4)
	{
		printk("%5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
	}
}

void prep_sgdata(struct scatterlist *sg)
{
	int i;
	unsigned int *d = sg_virt(sg);
	unsigned int dat;
	for(i = 0;i < sg->length + 16;i += 4)
	{
		dat = d[i/4];
		d[i/4] = __be32_to_cpu(dat);
	}
}

// static void debug_regs(struct ingenic_aes_dev *aes)
// {
// 	printk("AES_ASCR (0x00)= 0x%08lx\n", aes_read(aes, AES_ASCR));
// 	printk("AES_ASSR (0x04)= 0x%08lx\n", aes_read(aes, AES_ASSR));
// 	printk("AES_ASINTM (0x08)= 0x%08lx\n", aes_read(aes, AES_ASINTM));
// 	printk("AES_ASSA (0x0c)= 0x%08lx\n", aes_read(aes, AES_ASSA));
// 	printk("AES_ASDA (0x14)= 0x%08lx\n", aes_read(aes, AES_ASDA));
// 	printk("AES_ASTC (0x10)= 0x%08lx\n", aes_read(aes, AES_ASTC));
// 	printk("AES_ASDTC (0x18)= 0x%08lx\n", aes_read(aes, AES_ASDTC));
// 	printk("AES_ASBNUM (0x1c)= 0x%08lx\n", aes_read(aes, AES_ASBNUM));
// }

static int ingenic_aes_crypt_dma(struct crypto_tfm *tfm,
	struct scatterlist *in_sg, struct scatterlist *out_sg,int cryptlen)
{
	struct ingenic_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	struct ingenic_aes_dev *aes = ctx->aes;

	dma_addr_t dma_addr_in = sg_dma_address(in_sg);
	dma_addr_t dma_addr_out = sg_dma_address(out_sg);
	// int length = sg_dma_len(in_sg);
	int length = cryptlen;
	int blen = 0;

	unsigned int config;
//	dev_info(aes->dev,"len: %d\n", length);
#ifndef CONFIG_SOC_PRJ008
	if(check_keydone(aes) != 0){
		dev_err(aes->dev,"check key done failed!\n");
		return -EINPROGRESS;
	}
#endif
	config = aes_read(aes,AES_ASCR);

	aes->dma_size = length;
	aes_write(aes,AES_ASSA,dma_addr_in);
	aes_write(aes,AES_ASDA,dma_addr_out);

	blen = length / 16; // perblock is 128bit.

	if(length % 16)
		return -EINVAL;
#if defined(CONFIG_SOC_PRJ007)
	aes_write(aes,AES_ASTC,blen);
	aes_write(aes,AES_ASINTM,4);
#elif defined(CONFIG_SOC_PRJ008)
	aes_write(aes,AES_ASINTM,2);
	aes_write(aes, AES_ASBNUM, blen);
	/* set transfer count */
	aes_write(aes, AES_ASTC, length / 4);
	/* set destination transfer count */
	aes_write(aes, AES_ASDTC, length / 4);

#endif

#if 0	
	printk("start dma: in=0x%08llx out=0x%08llx len=%d totalen:%d,remain:%d blocks=%d,irq:%d\n",
		(unsigned long long)dma_addr_in,
		(unsigned long long)dma_addr_out,
		length, ctx->aes->total,ctx->aes->remaining,blen,ctx->aes->irq);
#endif
	config |= (ASCR_DMAE | ASCR_DMAS | ASCR_KEYS);
	aes_write(aes,AES_ASCR,config);
	// debug_regs(aes);
	// printk("start aes dma processing...\n");
	return 0;
}
static int ingenic_aes_crypt_dma_start(struct ingenic_aes_dev *aes)
{
	struct crypto_tfm *tfm = crypto_skcipher_tfm(crypto_skcipher_reqtfm(aes->req));
	int err, fast = 0, in, out;
	size_t count;
	dma_addr_t addr_in, addr_out;
	struct scatterlist *in_sg, *out_sg;
	int len32;

	// printk("start to crypt data, total=%d\n", aes->total);
	unsigned int ivoffset = aes->req->cryptlen - AES_BLOCK_SIZE;
	//这里的ivoffset整个需要加密文件的偏移，而不是单次加密的偏移
	// 但是实际上，这里都是按4k进行分段处理的，最后一个sg时，才可以更新iv。
	// printk("[%s]cryptlen;%d,ivoffset:%#x,total:%d\n",__func__,aes->req->cryptlen,ivoffset,aes->total);
	dev_dbg(aes->dev,"total: %d\n", aes->total);

	if (sg_is_last(aes->in_sg) && sg_is_last(aes->out_sg)) {
		/* check for alignment */
		in = IS_ALIGNED((unsigned long)aes->in_sg->offset, sizeof(unsigned long));
		out = IS_ALIGNED((unsigned long)aes->out_sg->offset, sizeof(unsigned long));

		// printk("(inoffset:%d:outoffset:%d)(in:%d, out:%d)\n",aes->in_sg->offset,aes->out_sg->offset, in, out);
		// fast = in && out;
	}
	if (fast)  {
		// count = sg_copy(&aes->in_sg, &aes->in_offset, aes->buf_in,aes->buflen, aes->total, 0);//这里是不是要加上
#ifdef CONFIG_HIGHMEM
		if (PageHighMem(sg_page(aes->in_sg)))
			kmap_high(sg_page(aes->in_sg));
		if (PageHighMem(sg_page(aes->out_sg)))
			kmap_high(sg_page(aes->out_sg));
#endif
		// sg_dma_len(aes->out_sg) = sg_dma_len(aes->in_sg);//指定输出长度==输入长度
		count = min(aes->remaining, sg_dma_len(aes->in_sg));
		count = min(count, sg_dma_len(aes->out_sg));
		// printk("********** use fast path aes->total:%d,remain:%d,insglen:%d,offset:%d,outsglen:%d,offset:%d buflen:%d**********\n",
		// 	aes->total, aes->remaining, sg_dma_len(aes->in_sg), aes->in_sg->offset, sg_dma_len(aes->out_sg), aes->out_sg->offset, aes->buflen);
		if (count != aes->remaining) {
			pr_err("request length != buffer length\n");
			return -EINVAL;
		}

		if(aes->flags & FLAGS_CBC){
			if(aes->flags & FLAGS_DECRYPT){
				int i;
				memcpy(aes->req->iv, sg_virt(aes->in_sg) + ivoffset, AES_BLOCK_SIZE);
				for(i = 0;i < AES_BLOCK_SIZE;i += 4)
					printk("[%s]    dec fast iv:0x%08x\n",__FUNCTION__,((unsigned int *)aes->req->iv)[i/4]);
			}
		}

		// prep_sgdata(aes->in_sg);
		// dump_sgdata(aes->in_sg);

		pr_debug("fast\n");
		err = dma_map_sg(aes->dev, aes->in_sg, 1, DMA_TO_DEVICE);

		if (!err) {
			dev_err(aes->dev, "dma_map_sg() error\n");
			return -EINVAL;
		}

		err = dma_map_sg(aes->dev, aes->out_sg, 1, DMA_FROM_DEVICE);
		if (!err) {
			dev_err(aes->dev, "dma_map_sg() error\n");
			dma_unmap_sg(aes->dev, aes->in_sg, 1, DMA_TO_DEVICE);
			return -EINVAL;
		}

		addr_in = sg_dma_address(aes->in_sg);
		addr_out = sg_dma_address(aes->out_sg);
		in_sg = aes->in_sg;
		out_sg = aes->out_sg;

		aes->flags |= FLAGS_FAST;

	} else {
		/* use cache buffers */
#if 0
		printk("0. insg len:%d offset:%d, outsg len:%d offset:%d buflen:%d,total:%d,remain:%d\n",
			aes->in_sg->length, aes->in_sg->offset,
			aes->out_sg->length, aes->out_sg->offset,
			aes->buflen, aes->total,aes->remaining);
#endif
		count = sg_copy(&aes->in_sg, &aes->in_offset, aes->buf_in,
			aes->buflen, aes->remaining, 0);

		// printk("sg_copy count:%d\n",count);
		len32 = DIV_ROUND_UP(count, DMA_MIN) * DMA_MIN;
		/*
		 * The data going into the AES module has been copied
		 * to a local buffer and the data coming out will go
		 * into a local buffer so set up local SG entries for
		 * both.
		 */
		sg_init_one(&aes->in_sgl,aes->buf_in,len32);
		sg_dma_len(&aes->in_sgl) = len32;
		sg_dma_address(&aes->in_sgl) = aes->dma_addr_in;
		sg_init_one(&aes->out_sgl,aes->buf_out,len32);
		sg_dma_len(&aes->out_sgl) = len32;
		sg_dma_address(&aes->out_sgl) = aes->dma_addr_out;
		// aes->out_sgl.offset = aes->in_sgl.offset;

		// printk("1. insg len:%d offset:%d, outsg len:%d offset:%d,count:%d,len32:%d,aesoffset:%d\n",
		// 	aes->in_sg->length, aes->in_sg->offset,
		// 	aes->out_sg->length, aes->out_sg->offset,
		// 	count, len32, aes->in_offset);
#if 0
		printk("2. in_sgl len:%d offset:%d, out_sgl len:%d offset:%d\n",
			aes->in_sgl.length, aes->in_sgl.offset,
			aes->out_sgl.length, aes->out_sgl.offset);
#endif
		in_sg = &aes->in_sgl;
		out_sg = &aes->out_sgl;

		addr_in = aes->dma_addr_in;
		addr_out = aes->dma_addr_out;

		if(aes->flags & FLAGS_CBC){
			if(aes->flags & FLAGS_DECRYPT){
				int i;
				memcpy(aes->req->iv, aes->buf_in + ivoffset, AES_BLOCK_SIZE);
				for(i = 0;i < AES_BLOCK_SIZE;i += 4)
					printk("[%s]   dec normal iv:0x%08x\n",__FUNCTION__,((unsigned int *)aes->req->iv)[i/4]);
			}
		}

#ifdef CONFIG_SOC_PRJ008
		prep_sgdata(in_sg);
#endif
		// dump_sgdata(in_sg);

		aes->flags &= ~FLAGS_FAST;
		dma_sync_sg_for_device(aes->dev, in_sg, 1, DMA_TO_DEVICE);
		dma_sync_sg_for_device(aes->dev, out_sg, 1, DMA_FROM_DEVICE);
	}

	aes->remaining -= count;
	// aes->total -= count;
	err = ingenic_aes_crypt_dma(tfm, in_sg, out_sg,count);
	if (err) {
		dma_unmap_sg(aes->dev, aes->in_sg, 1, DMA_TO_DEVICE);
		dma_unmap_sg(aes->dev, aes->out_sg, 1, DMA_FROM_DEVICE);
		printk("[%s] ingenic_aes_crypt_dma failed\n",__FUNCTION__);
	}

	// printk("%s succeed\n", __FUNCTION__);
	return err;
}

static void ingenic_aes_finish_req(struct ingenic_aes_dev *aes, int err)
{
	struct skcipher_request *req = aes->req;

	pr_debug("err: %d\n", err);

	aes->flags &= ~FLAGS_BUSY;
	req->base.complete(&req->base, err);
#if 0
	struct scatterlist *sg;
	int idx = 0,i = 0;
	printk("dst sg_nents:%d\n",sg_nents(req->dst));
	for_each_sg(req->dst, sg, sg_nents(req->dst), idx) {
		// void *base = sg_virt(sg) + sg->offset;
		// unsigned int *d = sg_virt(sg)+ (sg)->offset + count - 16;//;
		unsigned int *v = sg_virt(sg)+ (sg)->offset + (sg)->length - 16;//;
		unsigned int *k = sg_virt(sg);
		if(sg->length == 0){
			printk("idx:%d\n",idx);
			break;
		}
		for(i = 0;i < 16;i+=4){
			printk("---finish l:%d   %5d:   s0x%08x el0x%08x\n",(sg)->length, 
						i,__be32_to_cpu(k[i/4]),__be32_to_cpu(v[i/4]));
		}
		printk("outsg[%d]:%p offset=%d len=%d\n", idx,sg, sg->offset, sg->length);

	}

#endif

//#ifdef CONFIG_HIGHMEM
//	if (PageHighMem(sg_page(req->src)))
//		kunmap_high(sg_page(req->src));
//	if (PageHighMem(sg_page(req->dst)))
//		kunmap_high(sg_page(req->dst));
//#endif
}

static int ingenic_aes_crypt_dma_stop(struct ingenic_aes_dev *aes)
{
	int err = 0,i;
	size_t count;
	unsigned int ivoffset = aes->req->cryptlen - AES_BLOCK_SIZE;

	// printk("[%s]cryptlen;%d,ivoffset:%#x\n",__func__,aes->req->cryptlen,ivoffset);
	/* unsigned int val; */
	pr_debug("total: %d\n", aes->total);

	/* val = aes_read(aes,AES_ASCR); */
	/* val &= ~(3 << 8); */
	/* aes_write(aes,AES_ASCR,val); */

	if (aes->flags & FLAGS_FAST)
	{
		dma_unmap_sg(aes->dev, aes->out_sg, 1, DMA_FROM_DEVICE);
		// prep_sgdata(aes->out_sg);//网络字节序是大端，我们输出的数据是大端，但是cpu是小端，所以要先做一次转化，T33上可以通过配置进行修改
		printk("out_sg:\n");
		dump_sgdata(aes->out_sg);
		dma_unmap_sg(aes->dev, aes->in_sg, 1, DMA_TO_DEVICE);
		if(aes->flags & FLAGS_CBC){
			if(!(aes->flags & FLAGS_DECRYPT)){
				memcpy(aes->req->iv, sg_virt(aes->out_sg) + ivoffset, AES_BLOCK_SIZE);

				printk("[%s]enc fast iv\n",__func__);
				for(i = 0;i < AES_BLOCK_SIZE;i += 4)
				{
					printk("    %5d:0x%08x\n",i,((unsigned int *)aes->req->iv)[i/4]);
				}
			}
		}

#ifdef CONFIG_HIGHMEM
		if (PageHighMem(sg_page(aes->in_sg)))
			kunmap_high(sg_page(aes->in_sg));
		if (PageHighMem(sg_page(aes->out_sg)))
			kunmap_high(sg_page(aes->out_sg));
#endif
	} else {
		dma_sync_single_for_device(aes->dev, aes->dma_addr_out,
			aes->dma_size, DMA_FROM_DEVICE);

		// prep_sgdata(&aes->out_sgl);
		//dump_sgdata(&aes->out_sgl);
#if 0
		struct scatterlist *orig_out_sg = aes->out_sg;
		size_t orig_out_offset = aes->out_sg->offset;
		size_t dump_total_len = aes->dma_size; // 要dump的总长度（如4096字节）

		/* copy data */
		printk("4.1 outsg:%p outsg len:%d offset:%d buflen:%d,outoffset:%d\n",aes->out_sg,
			aes->out_sg->length, aes->out_sg->offset,
			aes->buflen,aes->out_offset);
#endif
		count = sg_copy(&aes->out_sg, &aes->out_offset, aes->buf_out,
			aes->buflen, aes->dma_size, 1);
#if 0
		// for(i = 0;i < 48;i+=4){
		// 	unsigned int *d = sg_virt(orig_out_sg);
		// 	printk("ori: %5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
		// }
		// for(i = 0;i < 48;i+=4){
		// 	unsigned int *d = sg_virt(orig_out_sg) + orig_out_offset -16;
		// 	printk("ori: %5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
		// }
		for(i = 0;i < 16;i+=4){
			unsigned int *d = aes->buf_out+ PAGE_SIZE - 16;//sg_virt(aes->out_sg);
			printk("buf_out: %5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
		}
		// if(!sg_is_last(aes->out_sg)){
		// 	for(i = 0;i < 16;i+=4){
		// 		unsigned int *d = sg_virt(aes->out_sg) + PAGE_SIZE - 16;//sg_virt(aes->out_sg);
		// 		printk("lastsg: %5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
		// 	}
		// }
		// printk("4.2  outsg len:%d offset:%d buflen:%d,dma_size:%d,count:%d,outoffset:%d\n",
		// 	aes->out_sg->length, aes->out_sg->offset,
		// 	aes->buflen, aes->dma_size, count,aes->out_offset);
#endif
		// if(sg_is_last(aes->out_sg)){
		// 	printk("outsg last 16 bytes data:\n");
		// 	unsigned int *d = sg_virt(aes->out_sg) + aes->out_sg->length -16;
		// 	for(i = 0;i < 16;i += 4)
		// 	{
		// 		printk("    %5d:0x%08x    0x%08x\n",i,d[i/4],__be32_to_cpu(d[i/4]));
		// 	}
		// }

		if(aes->flags & FLAGS_CBC){
			if(!(aes->flags & FLAGS_DECRYPT)){
				// wj:这里的iv应该不需要更新吧,暂时先屏蔽
#if 1
				ivoffset = aes->req->cryptlen - AES_BLOCK_SIZE;

				// memcpy(aes->req->iv, aes->buf_out + ivoffset, AES_BLOCK_SIZE);
				memcpy(aes->req->iv, aes->buf_out +aes->dma_size - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

				// printk("[%s]enc normal iv\n",__func__);
				// for(i = 0;i < AES_BLOCK_SIZE;i += 4)
				// {
				// 	printk("    %5d:0x%08x\n",i,((unsigned int *)aes->req->iv)[i/4]);
				// }
#endif
			}
		}
		if (count != aes->dma_size) {
			err = -EINVAL;
			pr_err("not all data converted: %u\n", count);
		}
	}
	return err;
}

static int ingenic_aes_start(struct ingenic_aes_dev *aes,
	struct skcipher_request *req)
{
	struct scatterlist *sg;
	int idx = 0;
	int srcsglen = 0,dstsglen = 0;

	struct crypto_async_request *async_req, *backlog;
	struct ingenic_aes_ctx *ctx;
	struct ingenic_aes_reqctx *rctx;
	unsigned long flags;
	int err, ret = 0;
	spin_lock_irqsave(&aes->lock, flags);
	if (req) {
//#ifdef CONFIG_HIGHMEM
//		if (PageHighMem(sg_page(req->src)))
//			kmap_high(sg_page(req->src));
//		if (PageHighMem(sg_page(req->dst)))
//			kmap_high(sg_page(req->dst));
//#endif
		ret = crypto_enqueue_request(&aes->queue, &req->base);
	}
	if (aes->flags & FLAGS_BUSY) {
		spin_unlock_irqrestore(&aes->lock, flags);
		return ret;
	}
	backlog = crypto_get_backlog(&aes->queue);
	async_req = crypto_dequeue_request(&aes->queue);
	if (async_req)
		aes->flags |= FLAGS_BUSY;
	spin_unlock_irqrestore(&aes->lock, flags);

	if (!async_req){
		ingenic_aes_stop(aes);
		return ret;
	}
	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);
	req = skcipher_request_cast(async_req);

	/* assign new request to device */
	aes->req = req;
	aes->total = req->cryptlen;
	aes->remaining = req->cryptlen;
	aes->in_offset = 0;
	aes->in_sg = req->src;
	aes->out_offset = 0;
	aes->out_sg = req->dst;

	for_each_sg(aes->in_sg, sg, sg_nents(aes->in_sg), idx) {
		// void *base = sg_virt(sg) + sg->offset;
		srcsglen += sg->length;
		// printk("insg[%d]:%p offset=%d len=%d\n", idx,sg, sg->offset, sg->length);
	}

	for_each_sg(aes->out_sg, sg, sg_nents(aes->out_sg), idx) {
		// void *base = sg_virt(sg) + sg->offset;
		dstsglen += sg->length;
		// printk("outsg[%d]:%p offset=%d len=%d\n", idx,sg, sg->offset, sg->length);
	}
	if(srcsglen != dstsglen){
		dev_err(aes->dev,"srcsglen:%d != dstsglen:%d\n",srcsglen,dstsglen);
		printk("************srcsglen:%d != dstsglen:%d*********\n",srcsglen,dstsglen);
		// return -EINVAL;

	}

	rctx = skcipher_request_ctx(req);
	ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	rctx->mode &= FLAGS_MODE_MASK;
	aes->flags = (aes->flags & ~FLAGS_MODE_MASK) | rctx->mode;

	aes->ctx = ctx;
	ctx->aes = aes;

	err = ingenic_aes_write_ctrl(aes);
	if (!err)
		err = ingenic_aes_crypt_dma_start(aes);

	if (err) {

		dev_err(aes->dev,"dma start failed.!\n");
		ingenic_aes_stop(aes);
		/* aes_task will not finish it, so do it here */
		ingenic_aes_finish_req(aes, err);
	}
	return ret; /* return ret, which is enqueue return value */
}

static int ingenic_aes_crypt(struct skcipher_request *req, unsigned long mode)
{
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(
		crypto_skcipher_reqtfm(req));
	struct ingenic_aes_reqctx *rctx = skcipher_request_ctx(req);
	struct ingenic_aes_dev *aes;
	pr_info("nbytes: %d, enc: %d, cbc: %d\n", req->cryptlen,
		!(mode & FLAGS_DECRYPT),
		!!(mode & FLAGS_CBC));

	if (!IS_ALIGNED(req->cryptlen, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks\n");
		return -EINVAL;
	}

	aes = ingenic_aes_find_dev(ctx);
	if (!aes)
		return -ENODEV;
	rctx->mode = mode;
	return ingenic_aes_start(aes, req);
}
static irqreturn_t ingenic_aec_irqthread(int irq, void *data)
{
	struct ingenic_aes_dev *aes = (struct ingenic_aes_dev *)data;
	unsigned long val;
	unsigned long mask;

	val = aes_read(aes,AES_ASSR);
	mask = aes_read(aes,AES_ASINTM);
	val = val & mask;

	// printk("irq come val: 0x%08lx,mask:%#lx\n", val,mask);
#if defined(CONFIG_SOC_PRJ007)
	if(val & 4){
		int err;
		err = ingenic_aes_crypt_dma_stop(aes);
		aes_write(aes,AES_ASSR,4);
		err = aes->err ? : err;
		if (aes->remaining && !err) {
			err = ingenic_aes_crypt_dma_start(aes);
			if (!err)
				return IRQ_HANDLED; /* DMA started. Not fininishing. */
		}
		ingenic_aes_finish_req(aes, err);
		ingenic_aes_start(aes,NULL);
	}else if(val & 2){
		aes_write(aes,AES_ASSR,2);
	}else if(val & 1){
		aes_write(aes,AES_ASSR,1);
	}else{
		dev_err(aes->dev,"unknown irq!!!\n");
		dump_aes_reg(aes);
	}
#else
	if(val & 2){
		int err;
		err = ingenic_aes_crypt_dma_stop(aes);
		aes_write(aes,AES_ASSR,2);
		err = aes->err ? : err;
		if (aes->remaining && !err) {
			err = ingenic_aes_crypt_dma_start(aes);
			if (!err)
				return IRQ_HANDLED; /* DMA started. Not fininishing. */
		}
		// printk("111111111111111111111111111\n");
		ingenic_aes_finish_req(aes, err);
		ingenic_aes_start(aes,NULL);
	} else if(val & 1){
		aes_write(aes,AES_ASSR,1);
	}else{
		dev_err(aes->dev,"unknown irq!!!\n");
		dump_aes_reg(aes);
	}
#endif

	return IRQ_HANDLED;
}

/* ********************** ALG API ************************************ */

static int ingenic_aes_setkey(struct crypto_skcipher *tfm, const u8 *key,
	unsigned int keylen)
{
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192 &&
		keylen != AES_KEYSIZE_256)
		return -EINVAL;

	pr_debug("enter, keylen: %d\n", keylen);

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int ingenic_aes_ecb_encrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, 0);
}

static int ingenic_aes_ecb_decrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, FLAGS_DECRYPT);
}

static int ingenic_aes_cbc_encrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, FLAGS_CBC);
}

static int ingenic_aes_cbc_decrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, FLAGS_DECRYPT | FLAGS_CBC);
}

static int ingenic_aes_cra_init(struct crypto_skcipher *tfm)
{
	pr_debug("enter\n");
	tfm->reqsize = sizeof(struct ingenic_aes_reqctx);
	return 0;
}

static void ingenic_aes_cra_exit(struct crypto_skcipher *tfm)
{
	pr_debug("exit\n");
}

/* ********************** ALGS ************************************ */
static struct skcipher_alg algs_ecb_cbc[] = {
	{
		.base.cra_name		= "ecb(aes)",
		.base.cra_driver_name	= "ecb-aes-ingenic",
		.base.cra_priority	= 100,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER | CRYPTO_ALG_ASYNC,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct ingenic_aes_ctx),
		.base.cra_alignmask	= 0,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.setkey			= ingenic_aes_setkey,
		.encrypt		= ingenic_aes_ecb_encrypt,
		.decrypt		= ingenic_aes_ecb_decrypt,

		.init			= ingenic_aes_cra_init,
		.exit			= ingenic_aes_cra_exit,
	},
	{
		.base.cra_name		= "cbc(aes)",
		.base.cra_driver_name	= "cbc-aes-ingenic",
		.base.cra_priority	= 100,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER | CRYPTO_ALG_ASYNC,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct ingenic_aes_ctx),
		.base.cra_alignmask	= 0,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.setkey			= ingenic_aes_setkey,
		.encrypt		= ingenic_aes_cbc_encrypt,
		.decrypt		= ingenic_aes_cbc_decrypt,

		.init			= ingenic_aes_cra_init,
		.exit			= ingenic_aes_cra_exit,
	},
};

static struct ingenic_aes_pdata ingenic_aes_pdata = {
	.algs_list	= algs_ecb_cbc,
	.size		= ARRAY_SIZE(algs_ecb_cbc),
};
static int ingenic_aes_get_res_pdev(struct ingenic_aes_dev *aes,
	struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *r;
	int err;
	int ret = 0;
	/* Get the base address */
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(dev, "no MEM resource info\n");
		err = -ENODEV;
		goto err;
	}

	aes->io_base = devm_ioremap(aes->dev, r->start, resource_size(r));
	if(IS_ERR_OR_NULL(aes->io_base)) {
		dev_err(&pdev->dev, "Failed to remap io resources!\n");
		ret = -ENOMEM;
		goto err;
	}

	aes->irq = platform_get_irq(pdev,0);
	if(aes->irq < 0){
		dev_err(dev,"Failed to request irq resource info\n");
		ret = -EBUSY;
		goto err;
	}

	err = request_threaded_irq(aes->irq,NULL,ingenic_aec_irqthread,
		IRQF_ONESHOT,dev_name(dev),aes);

	aes->pdata = &ingenic_aes_pdata;

err:
	return ret;
}
static noinline void pdma_wait(void)
{
	__asm__ volatile (
		"	.set	push		\n\t"
		"	.set	noreorder	\n\t"
		"	.set	mips32		\n\t"
		"	li	$26, 0		\n\t"
		"	mtc0	$26, $12	\n\t"
		"	nop			\n\t"
		"1:				\n\t"
		"	wait			\n\t"
		"	b	1b		\n\t"
		"	nop			\n\t"
		"	.set	reorder		\n\t"
		"	.set	pop		\n\t"
		);
}

static int ingenic_aes_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ingenic_aes_dev *aes;
	struct skcipher_alg *algp;
	int err = -ENOMEM, i;

	aes = devm_kzalloc(dev,sizeof(struct ingenic_aes_dev), GFP_KERNEL);
	if (aes == NULL) {
		dev_err(dev, "unable to alloc data struct.\n");
		goto err_data;
	}
	aes->dev = dev;
	aes->clk_gate = devm_clk_get(&pdev->dev, "gate_aes");
	if (IS_ERR(aes->clk_gate)) {
		dev_err(&pdev->dev, "cannot find RTC clock\n");
		err = PTR_ERR(aes->clk_gate);
		goto err_data;
	}

	clk_prepare_enable(aes->clk_gate);
	aes->clk_gate = devm_clk_get(&pdev->dev, "gate_pdma");
	if (IS_ERR(aes->clk_gate)) {
		dev_err(&pdev->dev, "cannot find aes clock\n");
		err = PTR_ERR(aes->clk_gate);
		goto err_data;
	}

	clk_prepare_enable(aes->clk_gate);
	printk("gate = %lx\n", *(unsigned long*)0xb0000020);

	platform_set_drvdata(pdev, aes);

	reset_mcu();
	memcpy((void*)MCU_BOOT,pdma_wait,64);
	boot_up_mcu();

	mdelay(100);
	/* *(unsigned long*)0xb3421030 = 1; */

	spin_lock_init(&aes->lock);
	crypto_init_queue(&aes->queue, INGENIC_AES_QUEUE_LENGTH);

	err = ingenic_aes_get_res_pdev(aes, pdev);
	if (err)
		goto err_data;

	err = ingenic_aes_dma_init(aes);
	if (err)
		goto err_data;

	INIT_LIST_HEAD(&aes->list);
	spin_lock(&list_lock);
	list_add_tail(&aes->list, &dev_list);
	spin_unlock(&list_lock);
	for (i = 0; i < aes->pdata->size; i++) {
			algp = &aes->pdata->algs_list[i];
			pr_info("reg alg: %s\n", algp->base.cra_name);
			err = crypto_register_skcipher(algp);
			if (err)
				goto err_algs;
			aes->pdata->registered++;
	}
	printk("ingenic aes[%d] register ok.",aes->pdata->registered);
	return 0;
err_algs:
	for (i = aes->pdata->registered - 1; i >= 0; i--)
		crypto_unregister_skcipher(&aes->pdata->algs_list[i]);
	ingenic_aes_dma_cleanup(aes);
err_data:
	dev_err(dev, "initialization failed.\n");
	return err;
}

static int ingenic_aes_remove(struct platform_device *pdev)
{
	struct ingenic_aes_dev *aes = platform_get_drvdata(pdev);
	int i;

	if (!aes)
		return -ENODEV;

	spin_lock(&list_lock);
	list_del(&aes->list);
	spin_unlock(&list_lock);
	for (i = aes->pdata->registered - 1; i >= 0; i--)
		crypto_unregister_skcipher(&aes->pdata->algs_list[i]);
	aes->pdata->registered = 0;
	ingenic_aes_dma_cleanup(aes);
	pm_runtime_disable(aes->dev);
	kfree(aes);
	aes = NULL;

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ingenic_aes_suspend(struct device *dev)
{
	pm_runtime_put_sync(dev);
	return 0;
}

static int ingenic_aes_resume(struct device *dev)
{
	pm_runtime_get_sync(dev);
	return 0;
}
#endif

static const struct dev_pm_ops ingenic_aes_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ingenic_aes_suspend, ingenic_aes_resume)
};

static const struct of_device_id ingenic_aes_dt_match[] = {
	{ .compatible = "ingenic,aes", .data = NULL },
	{},
};
// MODULE_DEVICE_TABLE(of, aic_dt_match);


static struct platform_driver ingenic_aes_driver = {
	.probe	= ingenic_aes_probe,
	.remove	= ingenic_aes_remove,
	.driver	= {
		.name	= "aes",
		.owner	= THIS_MODULE,
		.pm	= &ingenic_aes_pm_ops,
		.of_match_table = of_match_ptr(ingenic_aes_dt_match),
	},
};

module_platform_driver(ingenic_aes_driver);

MODULE_DESCRIPTION("Ingenic AES hw acceleration support.");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Stephon.Qiu");
