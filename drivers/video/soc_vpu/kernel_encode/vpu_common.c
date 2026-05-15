#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/delay.h>
#include "vpu_common.h"

void *vpu_wrap_malloc(unsigned int size)
{
	return vmalloc(size);
}

void vpu_wrap_free(void *addr)
{
	vfree(addr);
	return;
}

/* These APIs only valid for noncoherent memory */
void *vpu_wrap_malloc_linear(unsigned int size)
{
	dma_addr_t dma_handle = 0;
	return dma_alloc_noncoherent(NULL, size, &dma_handle, GFP_KERNEL | GFP_DMA);
}

void vpu_wrap_free_linear(void *addr, unsigned int size)
{
	return dma_free_noncoherent(NULL, size, addr, (dma_addr_t)vpu_wrap_vir2phy(addr));
}

unsigned int vpu_wrap_vir2phy(void *vir_addr)
{
	return virt_to_phys(vir_addr);
}

void *vpu_wrap_phy2vir(unsigned int phy_addr)
{
	return phys_to_virt(phy_addr);
}
/* Memory APIs end */

int vpu_wrap_write_reg(unsigned int reg, unsigned int val)
{
	writel(val, (void *)(reg + 0xa0000000));
	return 0;
}

int vpu_wrap_read_reg(unsigned int reg)
{
	return readl((void *)(reg + 0xa0000000));
}

/**
	direction:
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
*/
void vpu_wrap_flush_cache(void *vir_addr, unsigned int size, int direction)
{
	dma_cache_sync(NULL, vir_addr, size, direction);
	return;
}

void vpu_wrap_usleep(unsigned int usec)
{
	udelay(usec);
}

int c_clip3(int i_min, int i_max, int v)
{
	return ((v < i_min) ? i_min : ((v > i_max) ? i_max : v));
}
