#ifndef _OSD_H
#define _OSD_H
#include "../osd/ingenic_ipu_hal.h"
#include "../osd/imp_osd.h"

typedef enum {
	TX_ISP_IPU_OSD_MODE,
    TX_ISP_ISP_OSD_MODE,
    TX_ISP_BUTT_MODE,
} TX_ISP_OSD_MODE;

 typedef struct Date{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
 }Date;

 int isLeapYear(int year) ;
 void calculate_dates(Date *current_date, Date *last_date);
 int update_time(void *p);
 int update_thread(void *p);
int OSD_CreateRgn(int grpNum, uint32_t *timeStampData_ipu, int type);
#endif
