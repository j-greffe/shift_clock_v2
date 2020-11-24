#include "hal_gpio.h"
#include "shift_register_74hc595x.h"
#include "shift_digit.h"

#define IO_RFU2         HAL_IO_PACK(1, 0)
#define IO_RX           HAL_IO_PACK(1, 1)
#define IO_TX           HAL_IO_PACK(1, 2)
#define IO_BUTTON1      HAL_IO_PACK(1, 3)
#define IO_LIGHT        HAL_IO_PACK(1, 4)
#define IO_RFU7         HAL_IO_PACK(1, 5)
#define IO_RFU15        HAL_IO_PACK(1, 6)
#define IO_RFU16        HAL_IO_PACK(1, 7)

#define IO_ENA          HAL_IO_PACK(2, 1)
#define IO_SCK          HAL_IO_PACK(2, 0)
#define IO_SER          HAL_IO_PACK(2, 2)
#define IO_RCK          HAL_IO_PACK(2, 3)
#define IO_CLR          HAL_IO_PACK(2, 4)
#define IO_BUTTON2      HAL_IO_PACK(2, 5)
#define IO_XIN          HAL_IO_PACK(2, 6) // XXX Do not modify
#define IO_XOUT         HAL_IO_PACK(2, 7) // XXX Do not modify

extern const shift_register_74hc595x_hwcfg_t k_shift_register_hwcfg;

extern const shift_digit_hwcfg_t k_shift_digit_hwcfg;
