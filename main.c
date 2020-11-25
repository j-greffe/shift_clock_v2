#include <kal.h>
#include <msp430.h>
#include <shift_clock.h>
#include <stdlib.h>
#include <stdint.h>

#include "hwcfg.h"

#include "hal.h"
#include "main.h"

static date_t g_current;
static uint16_t g_adcLight;

static void time_increment(void* param)
{
    shift_clock_set_event(EVENT_MINUTE_INCREMENT);
}

static void light_measure_callback(void* adcValue)
{
    g_adcLight = (uint16_t)adcValue;
    shift_clock_set_event(EVENT_LIGHT_MEASURE);
}

void test_light_sensor(void)
{
    uint8_t event;

    hal_adc_start(INCH_4, light_measure_callback);
    hal_timer_A0_start(TIMER_BLINK, HAL_TIMER_CONTINOUS, shift_clock_event_blink, 0, 512);

    while (1)
    {
        event = shift_clock_wait_event(EVENT_ALL);

        if (EVENT_BLINK_ON == event)
        {
            // Read ADC value of light sensor
            hal_adc_start(INCH_4, light_measure_callback);
        }
        else if (EVENT_LIGHT_MEASURE == event)
        {
            //shift_clock_print_int(g_adcLight);
        }
    }
}

int main() {
    uint8_t event;
    bool time_is_valid = false;

    // Stop watchdog
    hal_wdg_stop();

    // Set unused pins
    hal_gpio_cfg(IO_RX, HAL_IO_NC);
    hal_gpio_cfg(IO_TX, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU2, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU7, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU14, HAL_IO_NC);
    hal_gpio_cfg(IO_RFU15, HAL_IO_NC);

    // Open timer A0
    hal_timer_A0_open();

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

    // Main timer. IRQ every 60s
    hal_timer_A0_start(TIMER_COUNT, HAL_TIMER_CONTINOUS, time_increment, 0, (uint16_t)60*1024);

    // Secondary timer. Semi-column blink and display update
    hal_timer_A0_start(TIMER_BLINK, HAL_TIMER_CONTINOUS, shift_clock_event_blink, 0, 512);

//    test_light_sensor();

    while(1) {

        event = shift_clock_wait_event(EVENT_ALL);

        if (EVENT_MINUTE_INCREMENT == event)
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
            hal_timer_A0_stop(TIMER_COUNT);
            hal_timer_A0_stop(TIMER_BLINK);

            shift_clock_input_new_time(&g_current);
            time_is_valid = true;

            shift_clock_print_time(&g_current, true);

            hal_timer_A0_start(TIMER_COUNT, HAL_TIMER_CONTINOUS, time_increment, 0, (uint16_t)60*1024);
            hal_timer_A0_start(TIMER_BLINK, HAL_TIMER_CONTINOUS, shift_clock_event_blink, 0, 512);
        }
        else if (EVENT_BLINK_ON == event)
        {
            shift_clock_print_time(&g_current, true);

            // Read ADC value of light sensor
            hal_adc_start(INCH_4, light_measure_callback);
        }
        else if (EVENT_BLINK_OFF == event)
        {
            if (time_is_valid)
            {
                shift_clock_print_time(&g_current, false);
            }
            else
            {
                shift_clock_print_blank();
            }
        }
        else if (EVENT_LIGHT_MEASURE == event)
        {
#define LIGHT_LOW   300
#define LIGHT_HIGH  500
#define LIGHT_STEP  30
#define LIGHT_MIN   10
            // Adjust brightness
            uint8_t percent;
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
            if (percent > percent_old + (LIGHT_STEP/2) || percent + (LIGHT_STEP/2) < percent_old || percent == 1 || percent == 100)
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
