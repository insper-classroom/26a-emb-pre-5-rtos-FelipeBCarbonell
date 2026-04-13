/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


#include <stdio.h>
#include <string.h> 
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;


QueueHandle_t xQueueButId;
QueueHandle_t xQueueLedR;
QueueHandle_t xQueueLedY;

void btn_callback(uint gpio, uint32_t events) {
    if (events == GPIO_IRQ_EDGE_FALL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (gpio == BTN_PIN_R || gpio == BTN_PIN_Y) {
            xQueueSendFromISR(xQueueButId, &gpio, &xHigherPriorityTaskWoken);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    bool led_on = false;

    while (true) {
        xQueueReceive(xQueueLedR, &led_on, 0);

        if (led_on) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled_with_callback(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    bool led_r_on = false;
    bool led_y_on = false;
    uint gpio = 0;

    while (true) {
        if (xQueueReceive(xQueueButId, &gpio, portMAX_DELAY) == pdTRUE) {
            if (gpio == BTN_PIN_R) {
                led_r_on = !led_r_on;
                xQueueOverwrite(xQueueLedR, &led_r_on);
            } else if (gpio == BTN_PIN_Y) {
                led_y_on = !led_y_on;
                xQueueOverwrite(xQueueLedY, &led_y_on);
            }
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    bool led_on = false;

    while (true) {
        xQueueReceive(xQueueLedY, &led_on, 0);

        if (led_on) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueButId = xQueueCreate(8, sizeof(uint));
    xQueueLedR = xQueueCreate(1, sizeof(bool));
    xQueueLedY = xQueueCreate(1, sizeof(bool));

    bool initial_state = false;
    xQueueOverwrite(xQueueLedR, &initial_state);
    xQueueOverwrite(xQueueLedY, &initial_state);

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
