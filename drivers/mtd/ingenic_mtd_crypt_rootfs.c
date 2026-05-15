/*
 * mtd-crypt-linux310_hardware.c - Linux 3.10 MTD decryption boot support (AES-ECB Hardware Accelerated)
 * Fixed for Ingenic hardware acceleration with DMA buffer handling
 * No key needed - uses pre-configured hardware key from eFuse
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/root_dev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/bio.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <crypto/internal/skcipher.h>
#include <linux/types.h>

extern bool is_mtd_crypt_mode;
/* ========== Asynchronous decryption context ========== */
struct mtdcrypt_async_ctx {
	struct completion done;
	int err;
};

/* ========== Configuration structure ========== */
struct mtd_crypt_config {
	char mtd_name[32];
	int enabled;
	u64 mtd_size;
	int force_sw;
	int debug_level;
};

struct mtd_crypt_config mtd_crypt_cfg = {
	.mtd_name = {0},
	.enabled = 0,
	.mtd_size = 0,
	.force_sw = 0,
	.debug_level = 1
};
EXPORT_SYMBOL(mtd_crypt_cfg);

/* ========== Ramdisk variables ========== */
static int mtdcrypt_major_num = 0;
static struct gendisk *mtdcrypt_ramdisk = NULL;
static struct request_queue *mtdcrypt_queue = NULL;
static unsigned char *mtdcrypt_ram_data = NULL;
static size_t mtdcrypt_ram_size = 0;

/* ========== Encryption variables ========== */
static struct crypto_ablkcipher *aes_tfm = NULL;
static struct mtd_info *mtd_device = NULL;

/* ========== Block device operations ========== */
static const struct block_device_operations mtdcrypt_fops = {
	.owner = THIS_MODULE,
};

/* ========== Completion callback ========== */
static void mtdcrypt_complete(struct crypto_async_request *req, int err)
{
	struct mtdcrypt_async_ctx *ctx = req->data;
	ctx->err = err;
	complete(&ctx->done);
}

/* ========== Ramdisk request handler ========== */
static void mtdcrypt_make_request(struct request_queue *q, struct bio *bio)
{
	int rw = bio_data_dir(bio);
	char *buffer = bio_data(bio);
	sector_t sector = bio->bi_sector;
	unsigned int nsectors = bio->bi_size >> 9;
	unsigned int offset = sector * 512;
	unsigned int size = nsectors * 512;

	if (offset + size > mtdcrypt_ram_size) {
		bio_endio(bio, -EIO);
		return;
	}

	if (rw == WRITE) {
		memcpy(mtdcrypt_ram_data + offset, buffer, size);
	} else {
		memcpy(buffer, mtdcrypt_ram_data + offset, size);
	}

	bio_endio(bio, 0);
}

/* ========== Create ramdisk device ========== */
static int create_mtdcrypt_ramdisk(void *data, size_t size)
{
	int ret;

	if (!data || size == 0) {
		printk(KERN_ERR "MTDCRYPT: Invalid ramdisk data\n");
		return -EINVAL;
	}

	mtdcrypt_ram_data = (unsigned char *)data;
	mtdcrypt_ram_size = size;

	mtdcrypt_major_num = register_blkdev(0, "mtdcrypt_ram");
	if (mtdcrypt_major_num < 0) {
		printk(KERN_ERR "MTDCRYPT: Failed to register block device: %d\n", mtdcrypt_major_num);
		return mtdcrypt_major_num;
	}

	mtdcrypt_queue = blk_alloc_queue(GFP_KERNEL);
	if (!mtdcrypt_queue) {
		printk(KERN_ERR "MTDCRYPT: Failed to create request queue\n");
		ret = -ENOMEM;
		goto unregister_blkdev;
	}

	blk_queue_make_request(mtdcrypt_queue, mtdcrypt_make_request);
	blk_queue_logical_block_size(mtdcrypt_queue, 512);
	blk_queue_max_hw_sectors(mtdcrypt_queue, 1024);

	mtdcrypt_ramdisk = alloc_disk(1);
	if (!mtdcrypt_ramdisk) {
		printk(KERN_ERR "MTDCRYPT: Failed to create device\n");
		ret = -ENOMEM;
		goto cleanup_queue;
	}

	mtdcrypt_ramdisk->major = mtdcrypt_major_num;
	mtdcrypt_ramdisk->first_minor = 0;
	mtdcrypt_ramdisk->minors = 1;
	mtdcrypt_ramdisk->fops = &mtdcrypt_fops;
	mtdcrypt_ramdisk->queue = mtdcrypt_queue;
	mtdcrypt_ramdisk->private_data = NULL;

	sprintf(mtdcrypt_ramdisk->disk_name, "mtdcrypt_ram");
	set_capacity(mtdcrypt_ramdisk, size / 512);
	add_disk(mtdcrypt_ramdisk);

	return 0;

cleanup_queue:
	if (mtdcrypt_queue) {
		blk_cleanup_queue(mtdcrypt_queue);
		mtdcrypt_queue = NULL;
	}

unregister_blkdev:
	if (mtdcrypt_major_num > 0) {
		unregister_blkdev(mtdcrypt_major_num, "mtdcrypt_ram");
		mtdcrypt_major_num = 0;
	}

	return ret;
}

/* ========== Asynchronous decryption function ========== */
static int decrypt_aes_ecb_async(const u8 *cipher, u8 *plain, size_t len)
{
	struct scatterlist sg_in, sg_out;
	struct ablkcipher_request *req;
	struct mtdcrypt_async_ctx ctx;
	int ret;

	if (!aes_tfm) {
		printk(KERN_ERR "MTDCRYPT: AES transform not initialized\n");
		return -EINVAL;
	}

	/* Verify length is multiple of AES block size (16 bytes) */
	if (len % 16 != 0) {
		printk(KERN_ERR "MTDCRYPT: Data length %zu not multiple of AES block size (16)\n", len);
		return -EINVAL;
	}

	/* Allocate async request */
	req = ablkcipher_request_alloc(aes_tfm, GFP_KERNEL);
	if (!req) {
		printk(KERN_ERR "MTDCRYPT: Failed to allocate ablkcipher request\n");
		return -ENOMEM;
	}

	init_completion(&ctx.done);
	ctx.err = -EINPROGRESS;

	/* Setup scatterlist - note DMA mapping usage */
	sg_init_one(&sg_in, cipher, len);
	sg_init_one(&sg_out, plain, len);

	/* Map DMA buffers */
	sg_dma_address(&sg_in) = dma_map_single(NULL, (void *)cipher, len, DMA_TO_DEVICE);
	sg_dma_address(&sg_out) = dma_map_single(NULL, plain, len, DMA_FROM_DEVICE);
	sg_dma_len(&sg_in) = len;
	sg_dma_len(&sg_out) = len;

	/* Setup async request */
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_SLEEP,
			mtdcrypt_complete, &ctx);

	ablkcipher_request_set_crypt(req, &sg_in, &sg_out, len, NULL);

	ret = crypto_ablkcipher_decrypt(req);
	if (ret == -EINPROGRESS || ret == -EBUSY) {
		wait_for_completion(&ctx.done);
		ret = ctx.err;
	}

	/* Unmap DMA buffers */
	dma_unmap_single(NULL, sg_dma_address(&sg_in), len, DMA_TO_DEVICE);
	dma_unmap_single(NULL, sg_dma_address(&sg_out), len, DMA_FROM_DEVICE);

	ablkcipher_request_free(req);

	return ret;
}

/* ========== Crypto initialization ========== */
static int init_crypto_ecb(void)
{
	struct crypto_ablkcipher *tfm;

	/* Try hardware acceleration first */
	if (!mtd_crypt_cfg.force_sw) {
		tfm = crypto_alloc_ablkcipher("ecb-aes-ingenic", 0, 0);

		if (!IS_ERR(tfm)) {
			/* 
			 * Key is already configured in hardware via eFuse,
			 * so we don't need to set it here
			 */
			aes_tfm = tfm;
			printk(KERN_INFO "MTDCRYPT: Hardware AES acceleration enabled (key from eFuse)\n");
			return 0;
		}
		printk(KERN_ERR "MTDCRYPT: Hardware AES not available: %ld\n", PTR_ERR(tfm));
	}

	/* Fallback to software implementation - not supported without key */
	printk(KERN_ERR "MTDCRYPT: Software AES not supported when using eFuse key\n");
	printk(KERN_ERR "MTDCRYPT: Hardware acceleration is required\n");
	return -ENOTSUPP;
}

/* ========== Kernel parameter parsing ========== */
static int __init mtdcrypt_setup(char *str)
{
	char *dev;
	char *sw_flag;
	char *debug_str;

	dev = str;

	/* Parse optional parameters */
	sw_flag = strchr(str, ',');
	if (sw_flag) {
		*sw_flag++ = '\0';

		debug_str = strchr(sw_flag, ',');
		if (debug_str) {
			*debug_str++ = '\0';
			if (kstrtoint(debug_str, 10, &mtd_crypt_cfg.debug_level)) {
				mtd_crypt_cfg.debug_level = 1;
			}
		}

		if (strcmp(sw_flag, "sw") == 0) {
			mtd_crypt_cfg.force_sw = 1;
			printk(KERN_WARNING "MTDCRYPT: Software mode forced (may not work with eFuse key)\n");
		}
	}

	strlcpy(mtd_crypt_cfg.mtd_name, dev, sizeof(mtd_crypt_cfg.mtd_name));
	mtd_crypt_cfg.enabled = 1;
	printk(KERN_INFO "MTDCRYPT: Enabled for device %s (using eFuse key)\n", dev);

	return 1;
}
__setup("mtdcrypt=", mtdcrypt_setup);

/* ========== MTD device acquisition ========== */
static int get_mtd_device_ecb(void)
{
	int i, retry;

	if (!mtd_crypt_cfg.enabled)
		return -ENODEV;

	for (retry = 0; retry < 5; retry++) {
		const char *test_names[] = {
			mtd_crypt_cfg.mtd_name,
			"mtd2",
			"mtdblock2",
			"root",
			NULL
		};

		for (i = 0; test_names[i]; i++) {
			mtd_device = get_mtd_device_nm(test_names[i]);
			if (!IS_ERR(mtd_device)) {
				mtd_crypt_cfg.mtd_size = mtd_device->size;
				printk(KERN_INFO "MTDCRYPT: Found MTD device: %s, size: %llu bytes\n", 
						test_names[i], mtd_crypt_cfg.mtd_size);
				return 0;
			}
		}

		if (retry < 4) {
			msleep(200);
		}
	}

	printk(KERN_ERR "MTDCRYPT: Failed to find MTD device\n");
	return -ENODEV;
}

/* ========== Stream decryption function ========== */
static void *decrypt_mtd_to_memory_ecb(void)
{
	u8 *plain_buf = NULL;
	u8 *cipher_chunk = NULL;
	dma_addr_t cipher_dma_handle;
	size_t total_size;
	loff_t offset;
	int ret;

	if (!mtd_device)
		return NULL;

	total_size = mtd_device->size;

	printk(KERN_INFO "MTDCRYPT: Decrypting %zu bytes using hardware AES with eFuse key...\n", total_size);
	is_mtd_crypt_mode = 1;
	printk(KERN_DEBUG "MTDCRYPT: Entering MTD crypt mode for filesystem decryption\n");

	/* Ensure size is multiple of AES block size */
	if (total_size % 16 != 0) {
		total_size = total_size - (total_size % 16);
	}

	plain_buf = vmalloc(total_size);
	if (!plain_buf) {
		printk(KERN_ERR "MTDCRYPT: Failed to allocate memory\n");
		goto error_exit;
	}

	/* Allocate DMA buffer - required for hardware encryption */
	cipher_chunk = dma_alloc_coherent(NULL, 4096, &cipher_dma_handle, GFP_KERNEL | GFP_DMA);
	if (!cipher_chunk) {
		printk(KERN_ERR "MTDCRYPT: Failed to allocate DMA buffer\n");
		goto error_exit;
	}

	for (offset = 0; offset < total_size; offset += 4096) {
		size_t chunk = total_size - offset;
		size_t retlen;

		if (chunk > 4096)
			chunk = 4096;

		/* Ensure chunk is 16-byte aligned */
		chunk = chunk - (chunk % 16);
		if (chunk == 0)
			continue;

		ret = mtd_read(mtd_device, offset, chunk, &retlen, cipher_chunk);
		if (ret) {
			printk(KERN_ERR "MTDCRYPT: Read error at offset %llu\n", 
					(unsigned long long)offset);
			goto error;
		}

		if (retlen != chunk) {
			chunk = retlen;
		}

		/* Ensure alignment again */
		chunk = chunk - (chunk % 16);
		if (chunk == 0)
			continue;

		ret = decrypt_aes_ecb_async(cipher_chunk, plain_buf + offset, chunk);
		if (ret) {
			printk(KERN_ERR "MTDCRYPT: Decryption error at offset %llu\n", 
					(unsigned long long)offset);
			goto error;
		}
	}

	/* Free DMA buffer */
	dma_free_coherent(NULL, 4096, cipher_chunk, cipher_dma_handle);
	is_mtd_crypt_mode = 0;
	printk(KERN_INFO "MTDCRYPT: Decryption completed\n");

	return plain_buf;

error:
	if (cipher_chunk)
		dma_free_coherent(NULL, 4096, cipher_chunk, cipher_dma_handle);
	if (plain_buf)
		vfree(plain_buf);

error_exit:
	is_mtd_crypt_mode = 0;
	printk(KERN_ERR "MTDCRYPT: Decryption failed, exiting MTD crypt mode\n");

	return NULL;
}

/* ========== Cleanup function ========== */
static void cleanup_resources(void)
{
	if (mtdcrypt_ramdisk) {
		del_gendisk(mtdcrypt_ramdisk);
		put_disk(mtdcrypt_ramdisk);
		mtdcrypt_ramdisk = NULL;
	}

	if (mtdcrypt_queue) {
		blk_cleanup_queue(mtdcrypt_queue);
		mtdcrypt_queue = NULL;
	}

	if (mtdcrypt_major_num > 0) {
		unregister_blkdev(mtdcrypt_major_num, "mtdcrypt_ram");
		mtdcrypt_major_num = 0;
	}

	if (mtd_device) {
		put_mtd_device(mtd_device);
		mtd_device = NULL;
	}

	if (aes_tfm) {
		crypto_free_ablkcipher(aes_tfm);
		aes_tfm = NULL;
	}
}

/* ========== Root filesystem setup ========== */
static int __init setup_decrypted_root(void)
{
	void *decrypted_data = NULL;
	int ret;

	if (!mtd_crypt_cfg.enabled) {
		printk(KERN_INFO "MTDCRYPT: Decryption not enabled\n");
		return 0;
	}

	printk(KERN_INFO "MTDCRYPT: Starting decryption using hardware AES with eFuse key...\n");

	/* Initialize cryptography */
	ret = init_crypto_ecb();
	if (ret != 0) {
		printk(KERN_ERR "MTDCRYPT: Crypto init failed\n");
		goto cleanup;
	}

	/* Get MTD device */
	ret = get_mtd_device_ecb();
	if (ret != 0) {
		printk(KERN_ERR "MTDCRYPT: No MTD device found\n");
		goto cleanup;
	}

	/* Decrypt data */
	decrypted_data = decrypt_mtd_to_memory_ecb();
	if (!decrypted_data) {
		printk(KERN_ERR "MTDCRYPT: Decryption failed\n");
		ret = -EIO;
		goto cleanup;
	}

	/* Create ramdisk from decrypted data */
	ret = create_mtdcrypt_ramdisk(decrypted_data, mtd_crypt_cfg.mtd_size);
	if (ret) {
		printk(KERN_ERR "MTDCRYPT: Failed to create ramdisk\n");
		vfree(decrypted_data);
		goto cleanup;
	}

	if (mtdcrypt_ramdisk) {
		/* Set root device */
		ROOT_DEV = MKDEV(mtdcrypt_major_num, 0);

		printk(KERN_INFO "MTDCRYPT: Root device set to /dev/%s\n", 
				mtdcrypt_ramdisk->disk_name);
		return 0;
	}

	ret = -EINVAL;

cleanup:
	printk(KERN_ERR "MTDCRYPT: Decryption failed\n");
	cleanup_resources();
	return ret;
}

/* ========== Execute before root filesystem mount ========== */
late_initcall(setup_decrypted_root);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux 3.10 MTD decryption boot support with AES-ECB (Hardware Accelerated with eFuse key)");
MODULE_AUTHOR("jz.zshi");
MODULE_VERSION("1.1");
