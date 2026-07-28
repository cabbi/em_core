#ifdef ESP_PLATFORM
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "em_defs.h"
#include "em_gpio.h"

void pinMode(int pin, int mode) {
    // Return early if the pin number is invalid
    if (!GPIO_IS_VALID_GPIO(pin)) return;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.intr_type = GPIO_INTR_DISABLE;

    if (mode == OUTPUT) {
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    } else if (mode == INPUT) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    } else if (mode == INPUT_PULLUP) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    }

    gpio_config(&io_conf);
}

void digitalWrite(int pin, int level) {
    gpio_set_level((gpio_num_t)pin, level);
}

int digitalRead(int pin) {
    return gpio_get_level((gpio_num_t)pin);
}

template<class T>
T readAnalog(uint8_t pin, uint16_t iterations, uint16_t iterationDelayMs) {
    uint16_t min = 65535, max = 0;
    uint32_t sum = 0;
    for (uint16_t i = 0; i < iterations; i++) {
        uint32_t ms = i == 0 ? 0 : iterationDelayMs;
        if (ms > 0) {
            tDelay(ms, true);
        }
        uint16_t adc = analogRead(pin);
        sum += adc;
        if (adc < min) min = adc;
        if (adc > max) max = adc;
    }
    return static_cast<T>(sum - max - min)/static_cast<T>(iterations-2);
}

// Global handle for ADC1 unit
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Array tracking if an ADC channel has already been configured
static bool adc1_chan_configured[SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)] = {false};

/**
 * Internal Helper: Maps a physical ESP32 GPIO pin to its corresponding ADC1 Channel.
 */
static esp_err_t gpio_to_adc1_channel(int gpio, adc_channel_t *channel) {
#if defined(CONFIG_IDF_TARGET_ESP32C6)
    // ESP32-C6 Map: GPIO 0 through 6 translate directly to Channel 0 through 6
    if (gpio >= 0 && gpio <= 6) {
        *channel = (adc_channel_t)gpio;       
        return ESP_OK;
    }
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    // ESP32-S3 Map: GPIO 1..10 -> CH 0..9
    if (gpio >= 1 && gpio <= 10) {
        *channel = (adc_channel_t)(gpio - 1); 
        return ESP_OK;
    }
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    // ESP32-C3 Map: GPIO 0..4 -> CH 0..4
    if (gpio >= 0 && gpio <= 4) {
        *channel = (adc_channel_t)gpio;       
        return ESP_OK;
    }
#else 
    // Classic ESP32 Map
    switch (gpio) {
        case 36: *channel = ADC_CHANNEL_0; return ESP_OK;
        case 37: *channel = ADC_CHANNEL_1; return ESP_OK;
        case 38: *channel = ADC_CHANNEL_2; return ESP_OK;
        case 39: *channel = ADC_CHANNEL_3; return ESP_OK;
        case 32: *channel = ADC_CHANNEL_4; return ESP_OK;
        case 33: *channel = ADC_CHANNEL_5; return ESP_OK;
        case 34: *channel = ADC_CHANNEL_6; return ESP_OK;
        case 35: *channel = ADC_CHANNEL_7; return ESP_OK;
        default: break;
    }
#endif
    return ESP_ERR_NOT_FOUND;
}

int analogRead(int pin) {
    adc_channel_t channel;
    
    // Map the GPIO pin to an ADC1 channel
    if (gpio_to_adc1_channel(pin, &channel) != ESP_OK) {
        printf("Error: Pin %d is not a valid ADC1 pin!\n", pin);
        return 0; 
    }

    // Unit Configuration
    if (adc1_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config;
        init_config.unit_id = ADC_UNIT_1;
        #if CONFIG_IDF_TARGET_ESP32C6
            init_config.clk_src = ADC_DIGI_CLK_SRC_DEFAULT;
        #elif CONFIG_IDF_TARGET_ESP32S3
            init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
        #else
            init_config.clk_src = 0; 
        #endif
        init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
        
        adc_oneshot_new_unit(&init_config, &adc1_handle); 
    }

    // Channel Configuration (No out-of-order errors)
    if (!adc1_chan_configured[channel]) {
        adc_oneshot_chan_cfg_t config;
        config.atten = ADC_ATTEN_DB_12;         // Fallback to DB_11 if DB_12 is missing
        config.bitwidth = ADC_BITWIDTH_DEFAULT; // 12-bit width (0-4095)
        
        adc_oneshot_config_channel(adc1_handle, channel, &config);
        adc1_chan_configured[channel] = true;
    }

    // Read the raw analog value
    int raw_value = 0;
    if (adc_oneshot_read(adc1_handle, channel, &raw_value) == ESP_OK) {
        return raw_value;
    }

    return 0;
}

#endif //ESP_PLATFORM