#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define ESP_INTR_FLAG_DEFAULT	0
#define LED_BUILD_IN 			23
#define BUTTON_ON				4
#define BUTTON_OFF				5
#define GPIO_OUTPUT_PIN_SEL  	((1ULL << LED_BUILD_IN))
#define GPIO_INPUT_PIN_SEL  	((1ULL << BUTTON_ON) | (1ULL << BUTTON_OFF) )

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    uint64_t time_db_on = 0, time_db_off = 0;
    bool db_on = false, db_off = false;
    int cnt = 0;
    while(1)
    {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
			if(esp_timer_get_time()/1000 - time_db_on > 250)
			{
				if(io_num == BUTTON_ON && gpio_get_level(io_num) == 0)
				{
						time_db_on = esp_timer_get_time()/1000;
						printf("GPIO[%d] intr, val: %d\r\n", io_num, gpio_get_level(io_num));
						printf("count: %d\r\n", cnt++);
						gpio_set_level(LED_BUILD_IN, 1);
				}
			}
			if(esp_timer_get_time()/1000 - time_db_off > 250)
			{
				if(io_num == BUTTON_OFF && gpio_get_level(io_num) == 0)
				{
						time_db_off = esp_timer_get_time()/1000;
						printf("GPIO[%d] intr, val: %d\r\n", io_num, gpio_get_level(io_num));
						printf("count: %d\r\n", cnt++);
						gpio_set_level(LED_BUILD_IN, 0);
				}

			}
       }
    }
}
void Init_gpio_output()
{
	  gpio_config_t io_conf = {};
	  io_conf.intr_type = GPIO_INTR_DISABLE;
	  io_conf.mode = GPIO_MODE_OUTPUT;
	  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	  io_conf.pull_down_en = 0;
	  io_conf.pull_up_en = 0;
	  gpio_config(&io_conf);
	  gpio_set_level(LED_BUILD_IN, 0);
}
void Init_gpio_input()
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_ON, gpio_isr_handler, (void*) BUTTON_ON);
    gpio_isr_handler_add(BUTTON_OFF, gpio_isr_handler, (void*) BUTTON_OFF);
}

void app_main(void)
{
	 printf("Start GPIO\r\n");
	 Init_gpio_output();
	 Init_gpio_input();

	 gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
	 xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
}
