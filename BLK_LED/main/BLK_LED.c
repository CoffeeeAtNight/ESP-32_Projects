#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_2
#define BTN_PIN GPIO_NUM_15
static const char *TAG = "AKI_ESP_32";

void configure_gpio(void);

void app_main(void)
{
  configure_gpio();
  ESP_LOGI(TAG, "Hellu, from Aki!");
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
  int lastButtonState = 0;
  int currentButtonState;

  while (1)
  {
    currentButtonState = gpio_get_level(BTN_PIN);

    if (currentButtonState != lastButtonState)
    {
      if (currentButtonState)
      {
        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(TAG, "Button input is 1");
      }
      else
      {
        gpio_set_level(LED_PIN, 1);
        ESP_LOGI(TAG, "Button input is 0");
      }
      lastButtonState = currentButtonState;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void configure_gpio() {
  gpio_config_t io_conf;
  // Disable interrupts
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // Set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  // Bit mask of the pin that you want to set
  io_conf.pin_bit_mask = (1ULL << BTN_PIN);
  // Disable pull-up resistor
  io_conf.pull_up_en = 1;
  // Enable pull-down resistor
  io_conf.pull_down_en = 0;
  // Configure GPIO with the given settings
  gpio_config(&io_conf);
}