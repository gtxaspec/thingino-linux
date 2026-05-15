#ifndef _VPU_COMMON_H_
#define _VPU_COMMON_H_

#include <linux/types.h>
#include <linux/kernel.h>

#define VPU_DYNAMIC_DEBUG
#define CACHE_ALL_SIZE (1024*1024)

//cpm opt
#define REG_CPM_CLKGATE0 0x10000020
#define REG_CPM_CLKGATE1 0x10000028
#define REG_CPM_VPU_DIV  0x10000030
#define REG_CPM_ISPA_DIV 0x10000094
#define BIT_CPM_VPU_CLOCKGATE 28
#define BIT_CPM_IVDC_CLOCKGATE 3
#define BIT_CPM_AHB1_CLOCKGATE 2

//ret value
#define VPU_SUCCESS   0
#define VPU_BUFFULL   0x80000000
#define VPU_NOTREADY  0x80000001
#define VPU_ERRCHN    0x80000002

#ifdef VPU_DYNAMIC_DEBUG
#define VPU_DEBUG(fmt, ...)			\
do {			\
	printk(KERN_DEBUG "%s()-%d: " fmt "", __func__, __LINE__, ##__VA_ARGS__);		\
} while(0)
#else
#define VPU_DEBUG(fmt, ...)			\
do {			\
} while(0)
#endif

#define VPU_INFO(fmt, ...)			\
do {			\
	printk(KERN_INFO "%s()-%d: " fmt "", __func__, __LINE__, ##__VA_ARGS__);		\
} while(0)

#define VPU_WARNING(fmt, ...)			\
do {			\
	printk(KERN_WARNING "%s()-%d: " fmt "", __func__, __LINE__, ##__VA_ARGS__);		\
} while(0)

#define VPU_ERROR(fmt, ...)			\
do {			\
	printk("%s()-%d: " fmt "", __func__, __LINE__, ##__VA_ARGS__);		\
} while(0)

#define VPU_CHECK(expr, ...) if (!(expr)) { \
	VPU_ERROR(__VA_ARGS__); \
}

typedef union {uint32_t i; uint16_t b[2]; uint8_t  c[4];} c_union32_t;
#define C_M32(src)			(((c_union32_t*)(src))->i)
#define C_ALIGN(x,a)		(((x)+((a)-1))&~((a)-1))
#define C_ALIGN_DOWN(x,a)	((x)&~((a)-1))
#define C_MIN(a,b)			((a)<(b) ? (a) : (b))
#define C_MAX(a,b)			((a)>(b) ? (a) : (b))
#define C_MIN3(a,b,c)		C_MIN((a),C_MIN((b),(c)))
#define C_MAX3(a,b,c)		C_MAX((a),C_MAX((b),(c)))
#define C_MIN4(a,b,c,d)		C_MIN((a),C_MIN3((b),(c),(d)))
#define C_MAX4(a,b,c,d)		C_MAX((a),C_MAX3((b),(c),(d)))
#define C_CLIP3(min,max,v)                   \
  {                                          \
    if((v)<(min))                               \
      (v)=(min);                                \
    else if((v)>(max))                          \
      (v)=(max);                                \
  }
#define RC_MEMCPY(dst,src) memcpy(dst, src, sizeof(dst))
#define RC_MEMSET(dst,src) memset(dst, src, sizeof(dst))

struct vpu_meminfo {
	unsigned int size;
	unsigned int align;
	unsigned int *vir_addr;
	unsigned int phy_addr;
};

void *vpu_wrap_malloc(unsigned int size);
void vpu_wrap_free(void *addr);
void *vpu_wrap_malloc_linear(unsigned int size);
void vpu_wrap_free_linear(void *addr, unsigned int size);
unsigned int vpu_wrap_vir2phy(void *vir_addr);
void *vpu_wrap_phy2vir(unsigned int phy_addr);
void vpu_wrap_flush_cache(void *vir_addr, unsigned int size, int direction);
int vpu_wrap_write_reg(unsigned int reg, unsigned int val);
int vpu_wrap_read_reg(unsigned int reg);
void vpu_wrap_usleep(unsigned int usec);
int c_clip3(int v, int i_min, int i_max);
#endif //_VPU_COMMON_H_
