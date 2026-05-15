#ifndef __ZROOTFS_RUN_H_
#define __ZROOTFS_RUN_H_
bool zroot_open(void);
int32_t zroot_wait_start(int is_wait);
int32_t zroot_is_done(int32_t timeout);
int32_t zroot_wait_done(uint32_t time_cnt);
int32_t zroot_failed_label(void);
#endif
