#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#include "stdio.h"

#define BUTTON_D3 0
#define BUTTON_D8 15
#define LED 2
#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TIME 200
#define TIMEOUT_1MIN 60000

enum interruptor_state {
  LED_ON,
  LED_OFF
};

long reboundTimeout;
long onTimeout;

int checkIfPressed(fsm_t*);
int checkTimeout(fsm_t*);
void led_on(fsm_t*);
void led_off(fsm_t*);
long getCurrentTime();

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

struct fsm_trans_t interruptor[] = {
  {LED_OFF, checkIfPressed, LED_ON, led_on},
  {LED_ON, checkTimeout, LED_OFF, led_off},
  {-1, NULL, -1, NULL},
};

void run(void* ignore)
{
    fsm_t* interruptor_fsm = fsm_new(interruptor);
    led_off(interruptor_fsm);


    portTickType xLastWakeTime;
    while (true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(interruptor_fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

int checkIfPressed(fsm_t* this) {
  return ((!GPIO_INPUT_GET(BUTTON_D3) || GPIO_INPUT_GET(BUTTON_D8)) && getCurrentTime() > reboundTimeout);
}

int checkTimeout(fsm_t* this) {
  return getCurrentTime() > onTimeout;
}

long getCurrentTime() {
  return xTaskGetTickCount()*portTICK_RATE_MS;
}

void led_on (fsm_t* this) {
  reboundTimeout = getCurrentTime() + REBOUND_TIME;
  onTimeout = getCurrentTime() + TIMEOUT_1MIN;
  GPIO_OUTPUT_SET(LED, 0);
}

void led_off (fsm_t* this) {
  reboundTimeout = getCurrentTime() + REBOUND_TIME;
  GPIO_OUTPUT_SET(LED, 1);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
  // Config pin as GPIO2
  PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO2);
  // Config pin as GPIO15
  PIN_FUNC_SELECT (GPIO_PIN_REG_15, FUNC_GPIO15);

  // Config pin as GPIO0
  PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO0);

  xTaskCreate(&run, "startup", 2048, NULL, 1, NULL);
}
