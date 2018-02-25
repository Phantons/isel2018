/**
 * Para este ejercicio optativo he supuesto que el sensor PIR se conecta al pin D2 (GPIO4)
 * y que cuando el sensor detecta movimiento en la sala pone la señal del GPIO a 1, será
 * 0 en caso de que no detecte a nadie.
 */

#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#include "stdio.h"

#define BUTTON_D3 0
#define BUTTON_D8 15
#define PIR_SENSOR 4
#define LED 2
#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TIME 300/portTICK_RATE_MS
#define TIMEOUT_1MIN 60000/portTICK_RATE_MS
#define ETS_GPIO_INTR_DISABLE() \
  _xt_isr_mask(1 << ETS_GPIO_INUM)
#define ETS_GPIO_INTR_ENABLE() \
  _xt_isr_unmask(1 << ETS_GPIO_INUM)

enum interruptor_state {
  LED_ON,
  LED_OFF
};

portTickType reboundTimeout;
portTickType onTimeout;
volatile bool pressed = false;
volatile bool isPresent = false;
int checkIfPressedOrPIRSensorDetectSomeone(fsm_t*);
int checkTimeout(fsm_t*);
void led_on(fsm_t*);
void led_off(fsm_t*);
void isr_gpio();
void clearFlags();

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
  {LED_OFF, checkIfPressedOrPIRSensorDetectSomeone, LED_ON, led_on},
  {LED_ON, checkIfPressedOrPIRSensorDetectSomeone, LED_ON, led_on},
  {LED_ON, checkTimeout, LED_OFF, led_off},
  {-1, NULL, -1, NULL},
};

void run(void* ignore)
{
    fsm_t* interruptor_fsm = fsm_new(interruptor);
    led_off(interruptor_fsm);

    gpio_intr_handler_register((void*)isr_gpio, NULL);
    gpio_pin_intr_state_set(BUTTON_D3, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(BUTTON_D8, GPIO_PIN_INTR_POSEDGE);
    gpio_pin_intr_state_set(PIR_SENSOR, GPIO_PIN_INTR_NEGEDGE);

    ETS_GPIO_INTR_ENABLE();

    portTickType xLastWakeTime;
    while (true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(interruptor_fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

int checkIfPressedOrPIRSensorDetectSomeone(fsm_t* this) {
  return pressed || isPresent;
}

int checkTimeout(fsm_t* this) {
  return xTaskGetTickCount() > onTimeout;
}

void isr_gpio() {
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  static bool isActive = false;
  long now = xTaskGetTickCount();
  // Atencion a la interrupcion del boton de encender luz
  if ((status & BIT(BUTTON_D3)) || status & BIT(BUTTON_D8)) {
    if (now > reboundTimeout) {
      reboundTimeout = now + REBOUND_TIME;
      pressed = true;
    }
    // Atencion a la interrupcion del pin del sensor de movimiento
  } else if (status & BIT(PIR_SENSOR)) {
    isPresent = true;
  }

  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

void led_on (fsm_t* this) {
  onTimeout = xTaskGetTickCount() + TIMEOUT_1MIN;
  GPIO_OUTPUT_SET(LED, 0);
  clearFlags();
}

void led_off (fsm_t* this) {
  GPIO_OUTPUT_SET(LED, 1);
  clearFlags();
}

void clearFlags() {
  isPresent = false;
  pressed = false;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
  // Config pin as GPIO15
  PIN_FUNC_SELECT (GPIO_PIN_REG_15, FUNC_GPIO15);


  xTaskCreate(&run, "startup", 2048, NULL, 1, NULL);
}
