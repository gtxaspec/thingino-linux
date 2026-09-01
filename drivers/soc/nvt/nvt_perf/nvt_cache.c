#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <asm/signal.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <asm/memory.h>
#include "../../arch/arm/mm/dma.h"
#include "nvt_perf_dbg.h"


#define ALLOC_PAGE_ORDER 10
#define ALLOC_SIZE (PAGE_SIZE * (1 << ALLOC_PAGE_ORDER))


#if (defined(CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL) || defined(CONFIG_NVT_CACHE_FLUSH_ALL_L2_CACHE))
extern unsigned long nvt_all_cache_op(void *vaddr, phys_addr_t paddr, u32 len, enum dma_data_direction dir, unsigned int retry_count);
#endif

static DEFINE_RAW_SPINLOCK(cache_lock);
int do_cache_perf(void)
{
	unsigned char* buff_va;
	phys_addr_t    buff_pa;
	unsigned int   op_size;
	unsigned int   size_index;
	struct timeval time_start;
	struct timeval time_stop;
	unsigned int total_sec;
	unsigned int total_usec;
#ifdef CONFIG_OUTER_CACHE
	struct timeval time_stop_l1;
	unsigned int total_sec_l1;
	unsigned int total_usec_l1;
#endif

	buff_va = (unsigned char*)__get_free_pages(GFP_KERNEL, ALLOC_PAGE_ORDER);
	if (buff_va == NULL) {
		nvt_dbg(ERR, "__get_free_pages failed\n");
		return -1;
	}

 	buff_pa =  __pa((unsigned long)buff_va);
	size_index = 1;

	raw_spin_lock(&cache_lock);

	/* cache operation via linux native API */
	total_sec = 0;
	total_usec = 0;
	op_size = 0x8000;

	/* Flush cache range*/
	printk("============== Flush cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

		dmac_flush_range(buff_va, buff_va + op_size);

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_flush_range(buff_pa, buff_pa + op_size);
#endif
		do_gettimeofday(&time_stop);

		total_sec = time_stop.tv_sec - time_start.tv_sec;
		total_usec = time_stop.tv_usec - time_start.tv_usec;

#ifdef CONFIG_OUTER_CACHE
		total_sec_l1 = time_stop_l1.tv_sec - time_start.tv_sec;
		total_usec_l1 = time_stop_l1.tv_usec - time_start.tv_usec;

		printk("cache flush size 0x%8x, Total(sec):%d.%06d l1(sec):%d.%06d l2(sec):%d.%06d\r\n", op_size, total_sec, total_usec, total_sec_l1, total_usec_l1, total_sec - total_sec_l1, total_usec -total_usec_l1);
#else
		printk("cache flush size 0x%8x, Total(sec):%d.%06d\r\n", op_size, total_sec, total_usec);
#endif
		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


	printk("\n");
	total_sec = 0;
	total_usec = 0;
	op_size = 0x8000;
	/* Flush cache range*/
	printk("============== clean cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

		dmac_map_area(buff_va, op_size, DMA_TO_DEVICE);

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_clean_range(buff_pa, buff_pa + op_size);
#endif
		do_gettimeofday(&time_stop);

		total_sec = time_stop.tv_sec - time_start.tv_sec;
		total_usec = time_stop.tv_usec - time_start.tv_usec;

#ifdef CONFIG_OUTER_CACHE
		total_sec_l1 = time_stop_l1.tv_sec - time_start.tv_sec;
		total_usec_l1 = time_stop_l1.tv_usec - time_start.tv_usec;

		printk("cache clean size 0x%8x, Total(sec):%d.%06d l1(sec):%d.%06d l2(sec):%d.%06d\r\n", op_size, total_sec, total_usec, total_sec_l1, total_usec_l1, total_sec - total_sec_l1, total_usec -total_usec_l1);
#else
		printk("cache clean size 0x%8x, Total(sec):%d.%06d\r\n", op_size, total_sec, total_usec);
#endif

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


	printk("\n");
	total_sec = 0;
	total_usec = 0;
	op_size = 0x8000;
	/* Flush cache range*/
	printk("============== Invalidate cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);


		do_gettimeofday(&time_start);
		dmac_map_area(buff_va, op_size, DMA_FROM_DEVICE);

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_inv_range(buff_pa, buff_pa + op_size);
#endif

		do_gettimeofday(&time_stop);

		total_sec = time_stop.tv_sec - time_start.tv_sec;
		total_usec = time_stop.tv_usec - time_start.tv_usec;

#ifdef CONFIG_OUTER_CACHE
		total_sec_l1 = time_stop_l1.tv_sec - time_start.tv_sec;
		total_usec_l1 = time_stop_l1.tv_usec - time_start.tv_usec;

		printk("cache invalidate size 0x%8x, Total(sec):%d.%06d l1(sec):%d.%06d l2(sec):%d.%06d\r\n", op_size, total_sec, total_usec, total_sec_l1, total_usec_l1, total_sec - total_sec_l1, total_usec -total_usec_l1);
#else
		printk("cache invalidate size 0x%8x, Total(sec):%d.%06d\r\n", op_size, total_sec, total_usec);
#endif
		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


#if (defined(CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL) || defined(CONFIG_NVT_CACHE_FLUSH_ALL_L2_CACHE))
	printk("\n");
	total_sec = 0;
	total_usec = 0;
	op_size = 0x8000;

	printk("============== flush cache all ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);
		nvt_all_cache_op((void *)buff_va, buff_pa, op_size, DMA_BIDIRECTIONAL, 10);

		do_gettimeofday(&time_stop);

		total_sec = time_stop.tv_sec - time_start.tv_sec;
		total_usec = time_stop.tv_usec - time_start.tv_usec;

		printk("cache flush size 0x%8x, Total(sec):%d.%06d\r\n", op_size, total_sec, total_usec);

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);

	printk("\n");
	total_sec = 0;
	total_usec = 0;
	op_size = 0x8000;

	printk("============== clean cache all ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);
		nvt_all_cache_op((void *)buff_va, buff_pa, op_size, DMA_TO_DEVICE, 10);
		do_gettimeofday(&time_stop);

		total_sec = time_stop.tv_sec - time_start.tv_sec;
		total_usec = time_stop.tv_usec - time_start.tv_usec;

		printk("cache clean size 0x%8x, Total(sec):%d.%06d\r\n", op_size, total_sec, total_usec);

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);

#else
	nvt_dbg(ERR, "The API of flush/clean cache all doesn't support by this kernel, so ignore related test\n");
#endif
	raw_spin_unlock(&cache_lock);

	free_pages((unsigned long)buff_va, ALLOC_PAGE_ORDER);
	return 0;
}
