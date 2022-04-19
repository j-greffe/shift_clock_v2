#include <msp430.h>
#include <stdlib.h>
#include <stdint.h>

#include "hwcfg.h"
#include "hal.h"
#include "kal.h"
#include "main.h"
#include "shift_clock.h"

static date_t g_current;
static uint16_t g_adcLight;
static bool g_blink_state;

static void display_refresh(void)
{
    shift_clock_set_event(EVENT_DISPLAY_REFRESH);
}

static void time_increment(void* param)
{
    shift_clock_set_event(EVENT_MINUTE_INCREMENT);
}
static void time_start(void)
{
    // Main timer. IRQ every 60s
    hal_timer_A0_start(TIMER_COUNT, HAL_TIMER_CONTINOUS, time_increment, 0, (uint16_t)60*1024);
}

static void time_stop(void)
{
    hal_timer_A0_stop(TIMER_COUNT);
}

static void blink_callback(void* param)
{
    g_blink_state = !g_blink_state;
    display_refresh();
}

static void blink_start(void)
{
    g_blink_state = true;
    // Secondary timer. Semi-column blink and display update
    hal_timer_A0_start(TIMER_BLINK, HAL_TIMER_CONTINOUS, blink_callback, 0, 512);
}

static void blink_stop(void)
{
    hal_timer_A0_stop(TIMER_BLINK);
}

static void light_measure_callback(void* adcValue)
{
    g_adcLight = (uint16_t)adcValue;
    shift_clock_set_event(EVENT_LIGHT_MEASURE);
}

void shift_clock_print_int(uint16_t i)
{
    shift_digit_print_char((i % 10) + '0', false); i /= 10;
    shift_digit_print_char((i % 10) + '0', false); i /= 10;
    shift_digit_print_char((i % 10) + '0', false); i /= 10;
    shift_digit_print_char(i + '0', false);
    shift_digit_update();
}

int main() {
    uint8_t event;
    bool time_is_valid = false;

    // Stop watchdog
    hal_wdg_stop();

    // Clock to max frequency
    hal_clk_open();

    // Open timer A0
    hal_timer_A0_open();

    // Set unused pins
    hal_gpio_cfg(IO_RX, HAL_IO_NC);
    hal_gpio_cfg(IO_TX, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU2, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU7, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU14, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU15, HAL_IO_NC);

    // Open KAL
    kal_dbg_open(HAL_IO_NONE);
    kal_event_open(EVENT_QUANTITY);

    // Open clock driver
    shift_clock_open();

    // Configure button1, no need for pull-up, hardware debounce.
    hal_gpio_cfg(IO_BUTTON1, HAL_IO_IN);
    hal_irq_set(IO_BUTTON1, HAL_IRQ_FALLING, shift_clock_event_button, (void*)EVENT_BUTTON1_PUSH);
    hal_irq_en(IO_BUTTON1);

    // Configure button2, no need for pull-up, hardware debounce.
    hal_gpio_cfg(IO_BUTTON2, HAL_IO_IN);
    hal_irq_set(IO_BUTTON2, HAL_IRQ_FALLING, shift_clock_event_button, (void*)EVENT_BUTTON2_PUSH);
    hal_irq_en(IO_BUTTON2);

    __enable_interrupt();

    // Test all segments
    shift_clock_print_full();
    shift_clock_wait(TIMER_BLINK, 1024);

#if 0

    while (1)
    {
        hal_adc_start(INCH_4, light_measure_callback);

        event = shift_clock_wait_event(EVENT_ALL);

        shift_clock_print_int(g_adcLight);

        shift_clock_wait(TIMER_BLINK, 1024);
    }
#endif

    time_start();
    blink_start();

    while(1) {

        event = shift_clock_wait_event(EVENT_ALL);

        if (EVENT_DISPLAY_REFRESH == event)
        {
            // Read ADC value of light sensor
            hal_adc_start(INCH_4, light_measure_callback);

            if (time_is_valid)
            {
                shift_clock_print_time(&g_current, g_blink_state);
            }
            else
            {
                if (g_blink_state)
                {
                    shift_clock_print_time(&g_current, true);
                }
                else
                {
                    shift_clock_print_blank();
                }
            }
        }
        else if (EVENT_MINUTE_INCREMENT == event)
        {
            g_current.minute++;
            if (g_current.minute > 59)
            {
                g_current.hour++;
                g_current.minute = 0;
                //shift_clock_set_event(EVENT_HOUR_INCREMENT);
            }

            if (g_current.hour > 23)
            {
                g_current.hour = 0;
                //shift_clock_set_event(EVENT_DAY_INCREMENT);
            }
        }
        else if (EVENT_BUTTON1_LONG_PRESS == event)
        {
            time_stop();
            blink_stop();

            shift_clock_input_new_time(&g_current);
            time_is_valid = true;

            time_start();
            blink_start();
            display_refresh();
        }
        else if (EVENT_BUTTON2_RELEASE == event)
        {
            blink_stop();

            // Test all segments
            shift_clock_print_full();
            shift_clock_wait(TIMER_BLINK, 1024);

            blink_start();
            display_refresh();
        }
        else if (EVENT_LIGHT_MEASURE == event)
        {
            uint8_t percent;

#define LIGHT_LOW   500
#define LIGHT_HIGH  900
#define LIGHT_STEP  30
#define LIGHT_MIN   1

            // Adjust brightness
            static uint8_t percent_old;

            if (g_adcLight < LIGHT_LOW)
            {
                percent = 1;
            }
            else if (g_adcLight > LIGHT_HIGH)
            {
                percent = 100;
            }
            else
            {
                percent = ((g_adcLight - LIGHT_LOW) * 100) / (LIGHT_HIGH - LIGHT_LOW);
            }

            // Update only if light change is significant
            if (percent != percent_old && (percent > percent_old + (LIGHT_STEP/2) || percent + (LIGHT_STEP/2) < percent_old || percent == 1 || percent == 100))
            {
                // Save new value
                percent_old = percent;

                // Use steps
                if (100 != percent)
                {
                    percent = ((percent + (LIGHT_STEP / 2)) / LIGHT_STEP) * LIGHT_STEP;
                }

                // Floor value
                if (percent < LIGHT_MIN)
                {
                    percent = LIGHT_MIN;
                }

                shift_clock_brightness_set(percent);
            }
        }
    }
}
