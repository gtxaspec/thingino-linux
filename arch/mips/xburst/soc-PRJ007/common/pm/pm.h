#ifndef __PM_H__
#define __PM_H__
#include <soc/base.h>
#define FASTBOOT_SLEEP_CODE_ADDR	(0xb2608000)//(0xb2210000)
#define FASTBOOT_SLEEP_CODE_LEN		(4 * 1024)
void load_func_to_tcsm(unsigned int tcsm_addr,unsigned int *f_addr,unsigned int size);
long long save_goto(unsigned int func);
int restore_goto(unsigned int func);
extern uint8_t *mem_test_rgn;
extern int bc_idx;

int __attribute__((weak)) ingenic_zboost_init(void)
{
    return 0;
}

int __attribute__((weak)) ingenic_zboost_exit(void)
{
    return 0;
}

#endif/*PM*/
