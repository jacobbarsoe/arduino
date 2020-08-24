#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "config.h"
#include "configure_rmt.h"

#include "driver/gpio.h"
#include <driver/dac.h>

static const char* TAG = "main";

int16_t scaleReceiver(uint16_t input, int16_t out_min, int16_t out_max) {
    // takes in a value from RECEIVER_CH_MIN to RECEIVER_CH_MAX centered on RECEIVER_CH_CENTER
    // and returns direction and speed
    // map function taken from https://www.arduino.cc/reference/en/language/functions/math/map/

    // if input out of range force it to be within
    if (input < RECEIVER_CH_MIN) {
        input = RECEIVER_CH_MIN;
    } else if (input > RECEIVER_CH_MAX) {
        input = RECEIVER_CH_MAX;
    }
    
    // if input within deadzone, return 0
    if (input > (RECEIVER_CH_CENTER-RECEIVER_CH_DEADZONE) && input < (RECEIVER_CH_CENTER+RECEIVER_CH_DEADZONE)) {
        return 0;
    }

    return (input - RECEIVER_CH_MIN) * (out_max - out_min) / (RECEIVER_CH_MAX - RECEIVER_CH_MIN) + out_min;
}

void ppm_task(void *pvParameter) {
    while(1) {
        int ppm1 = scaleReceiver(ReceiverChannels[0],0,255);
        int ppm2 = scaleReceiver(ReceiverChannels[1],0,255);
        int ppm3 = scaleReceiver(ReceiverChannels[2],0,255);

        dac_output_voltage(DAC_CHANNEL_1, ppm1);
        dac_output_voltage(DAC_CHANNEL_2, ppm2);
        ;
        ESP_LOGI(TAG, "%d %d %d",
            scaleReceiver(ReceiverChannels[0],0,4095),
            scaleReceiver(ReceiverChannels[1],0,4095),
            scaleReceiver(ReceiverChannels[2],0,4095)
        );        
        vTaskDelay(100 / portTICK_PERIOD_MS);

    }
}

void app_main()
{
    ESP_LOGI(TAG, "Started");
    rmt_init();
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_2);
    xTaskCreate(&ppm_task, "ppm_task", 2048, NULL, 5, NULL);
}
