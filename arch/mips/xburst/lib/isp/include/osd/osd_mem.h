
#ifndef __OSD_MEM_H__
#define __OSD_MEM_H__


#define TRUE 1
#define FALSE 0


struct mem_node_s;
struct mem_list_s;
struct mem_alloc_info_s;

typedef struct mem_node_s {
    struct mem_list_s* list;
    struct mem_node_s* next;
    struct mem_node_s* priv;
	int32_t busedflag;
	void* oldaddr;
    void* addr;
    int32_t size;
} mem_node_t;

typedef struct mem_list_s {
    int32_t cnt;
    mem_node_t* head;
    mem_node_t* tail;
    struct mem_alloc_info_s *meminfo;
} mem_list_t;

typedef struct mem_alloc_info_s {
   // mem_list_t free;
    uint32_t brmemsetbak;
    mem_list_t busy;
    void* start_addr;
    int32_t total_size;

} mem_alloc_info_t;

mem_alloc_info_t* OSD_mem_create(void* start_addr, int32_t size);
int OSD_mem_destroy(mem_alloc_info_t *mem);
void* OSD_mem_alloc(mem_alloc_info_t* minfo, int32_t size);
int32_t OSD_mem_try_alloc(mem_alloc_info_t* minfo, int32_t size);
int32_t OSD_mem_free(mem_alloc_info_t* minfo, void* start_addr);

#endif
