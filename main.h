#ifndef _MAIN_H_
#define _MAIN_H_

enum {
    MODE_TIME,
    MODE_YEAR,
    MODE_DATE,
    MODE_LOW_POWER,
};

void clock_low_power_enter(void);
void clock_low_power_exit(void* param);

#endif // _MAIN_H_
