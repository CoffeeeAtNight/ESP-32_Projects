#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"

static const char *TAG = "AKI_ESP32";

#define JOYSTICK_X_AXIS ADC1_CHANNEL_6
#define JOYSTICK_Y_AXIS ADC1_CHANNEL_7
#define JOYSTICK_BUTTON GPIO_NUM_32

#define SERVO_MIN_PULSEWIDTH_US 500  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2500 // Maximum pulse width in microsecond
#define SERVO_MIN_DEGREE -90         // Minimum angle
#define SERVO_MAX_DEGREE 90          // Maximum angle

#define SERVO_PULSE_GPIO 18                  // GPIO connects to the PWM signal line
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 20000          // 20000 ticks, 20ms

static inline uint32_t example_angle_to_compare(int angle)
{
  return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

void app_main(void)
{
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(JOYSTICK_X_AXIS, ADC_ATTEN_DB_6);
  adc1_config_channel_atten(JOYSTICK_Y_AXIS, ADC_ATTEN_DB_6);

  gpio_set_direction(JOYSTICK_BUTTON, GPIO_MODE_INPUT);

  ESP_LOGI(TAG, "Create timer and operator");
  mcpwm_timer_handle_t timer = NULL;
  mcpwm_timer_config_t timer_config = {
      .group_id = 0,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
      .period_ticks = SERVO_TIMEBASE_PERIOD,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
  };
  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

  mcpwm_oper_handle_t oper = NULL;
  mcpwm_operator_config_t operator_config = {
      .group_id = 0, // operator must be in the same group to the timer
  };
  ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

  ESP_LOGI(TAG, "Connect timer and operator");
  ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

  ESP_LOGI(TAG, "Create comparator and generator from the operator");
  mcpwm_cmpr_handle_t comparator = NULL;
  mcpwm_comparator_config_t comparator_config = {
      .flags.update_cmp_on_tez = true,
  };
  ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

  mcpwm_gen_handle_t generator = NULL;
  mcpwm_generator_config_t generator_config = {
      .gen_gpio_num = SERVO_PULSE_GPIO,
  };
  ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

  // set the initial compare value, so that the servo will spin to the center position
  ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(0)));

  ESP_LOGI(TAG, "Set generator action on timer and compare event");
  // go high on counter empty
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
  // go low on compare threshold
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                              MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

  ESP_LOGI(TAG, "Enable and start timer");
  ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
  ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

  double angle = 0.0;

  while (1)
  {
    int x_val = adc1_get_raw(JOYSTICK_X_AXIS);
    int y_val = adc1_get_raw(JOYSTICK_Y_AXIS);
    int button_pressed = gpio_get_level(JOYSTICK_BUTTON);

    // No need to round unless there's a specific requirement
    angle = atan2((double)y_val, (double)x_val); // Casting to double
    angle = angle * (180.0 / M_PI);              // Convert to degrees

    printf("X: %d, Y: %d, Button: %d\n", x_val, y_val, button_pressed);

    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Angle of rotation: %f", angle); // %f for double
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(angle)));

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
