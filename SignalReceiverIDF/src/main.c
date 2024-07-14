#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp32/rom/ets_sys.h"
#include "esp_task_wdt.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define kClockPin GPIO_NUM_27
#define kDataPin GPIO_NUM_26
#define kProcessPin GPIO_NUM_2
#define kSignalPin GPIO_NUM_19

const uint32_t kSerialBaudRate = 576000;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;

const uint32_t kClockLineMaxSize = 16;

const uint32_t kClockTimeout = 500000000;

void delay_ms(uint32_t ms);
void gpio_init();
void system_init();
inline uint32_t IRAM_ATTR clocks();

// globals
uint8_t clockLineCounter = 0;
uint32_t clockTimeout = 0;

uint32_t wdt_timeout_counter = 0;

char transmitBuffer[4];

// Main application function
void app_main()
{
    register int level, old_level = 0, data_line = 0;

    register uint32_t check_timeout = 0;

    register uint32_t check_timeout_frame_size = 0;

    register uint16_t transmit_data = 0;

    delay_ms(kDelayStartMs);
    system_init();
    gpio_init(); // Initialize GPIO

    while (1)
    {
        level = gpio_get_level(kClockPin);

        if ((level != old_level) && (old_level == 0))
        {
            data_line = gpio_get_level(kDataPin);
            transmit_data <<= 1;
            if (!clockLineCounter)
            {
                gpio_set_level(kSignalPin, 1);
                transmit_data = 0;
                clockTimeout = clocks();
                clockLineCounter = 1;
                if (data_line)
                    transmit_data = 1;
            }
            else
            {
                clockTimeout = clocks() - clockTimeout;
                if (clockTimeout > kClockTimeout)
                {
                    transmit_data = 0;
                    clockLineCounter = 0;
                    gpio_set_level(kSignalPin, 0);
                    // clockTimeout = clocks();
                    // clockLineCounter = 0;
                    // if (data_line)
                    // {
                    //   transmit_data = 1;
                    // }

                    ets_delay_us(10);
                }
                else
                {
                    clockTimeout = clocks();
                    clockLineCounter++;
                    if (data_line)
                    {
                        transmit_data += 1;
                    }
                    if (clockLineCounter >= kClockLineMaxSize)
                    {
                        clockLineCounter = 0;
                        gpio_set_level(kSignalPin, 0);
                        gpio_set_level(kProcessPin, 1);
                        // transmitBuffer[0] = (transmit_data >> 8) & 0xFF;
                        // transmitBuffer[1] = transmit_data & 0xFF;
                        // Serial.write(transmitBuffer);
                        gpio_set_level(kProcessPin, 0);
                    }
                }
            }
        }
        old_level = level;
    }
}

// Simple delay function
void delay_ms(uint32_t ms)
{
    ets_delay_us(ms * 1000);
}

// GPIO initialization function
void gpio_init()
{
    // Set the GPIO as an input
    gpio_set_direction(kClockPin, GPIO_MODE_INPUT);
    gpio_set_direction(kDataPin, GPIO_MODE_INPUT);

    gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(kSignalPin, GPIO_MODE_OUTPUT);
}

void system_init()
{
    esp_task_wdt_deinit();
}

inline uint32_t IRAM_ATTR clocks()
{
    uint32_t ccount;
    asm volatile("rsr %0, ccount" : "=a"(ccount));
    return ccount;
}
