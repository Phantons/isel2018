#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define BUTTON_D3 0
#define PIN_D8 15
#define LED 2
#define ALARM_SIGNAL_PORT 5
#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TIME 300
#define TIMEOUT 1000
#define ETS_GPIO_INTR_DISABLE() \
  _xt_isr_mask(1 << ETS_GPIO_INUM)
#define ETS_GPIO_INTR_ENABLE() \
  _xt_isr_unmask(1 << ETS_GPIO_INUM)



enum alarm_state {
  ALARM_OFF,
  ACT_1,
  ACT_2,
  ALARM_ON,
  DESACT_1,
  DESACT_2,
};

long reboundTimeout = 0;
long timeout = 0;
volatile int counter = 0;
volatile int pressed = false;
int checkIfPressed(fsm_t*);
int checkTimeout(fsm_t*);
void led_on();
void led_off(fsm_t*);
void setCounter(fsm_t* this);
void setPressed(fsm_t* this);
void isr_gpio();
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

struct fsm_trans_t alarm[] = {
  {ALARM_OFF, checkIfPressed, ACT_1, setPressed},
  {ACT_1, checkIfPressed, ACT_2, setPressed},
  {ACT_1, checkTimeout, ALARM_OFF, setCounter},
  {ACT_2, checkIfPressed, ALARM_ON, setPressed},
  {ACT_2, checkTimeout, ALARM_OFF, setCounter},
  {ALARM_ON, checkIfPressed, DESACT_1, setPressed},
  {DESACT_1, checkIfPressed, DESACT_2, setPressed},
  {DESACT_1, checkTimeout, ALARM_ON, setCounter},
  {DESACT_2, checkIfPressed, ALARM_OFF, led_off},
  {DESACT_2, checkTimeout, ALARM_ON, setCounter},
  {-1, NULL, -1, NULL},
};

void run(void* ignore)
{
    fsm_t* alarm_fsm = fsm_new(alarm);
    led_off(alarm_fsm);


    gpio_intr_handler_register((void*)isr_gpio, NULL);
    gpio_pin_intr_state_set(BUTTON_D3, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(PIN_D8, GPIO_PIN_INTR_POSEDGE);

    ETS_GPIO_INTR_ENABLE();

    portTickType xLastWakeTime;
    while (true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(alarm_fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

int checkIfPressed(fsm_t* this) {
  return pressed;
}

int checkTimeout(fsm_t* this) {
  if (getCurrentTime() > timeout)
  return getCurrentTime() > timeout;
}

void led_on () {
  GPIO_OUTPUT_SET(LED, 0);
}

void led_off (fsm_t* this) {
  GPIO_OUTPUT_SET(LED, 1);
  pressed = false;
}

void setCounter(fsm_t* this) {
  counter = 0;
}

void setPressed(fsm_t* this) {
  pressed = false;
}

long getCurrentTime() {
  return xTaskGetTickCount()*portTICK_RATE_MS;
}

void isr_gpio() {
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  static bool isActive = false;
  long now = getCurrentTime();
  // Atencion a la interrupcion del boton de armar/desarmar
  if (status & BIT(BUTTON_D3)) {
    if (now > reboundTimeout) {
      timeout = now + TIMEOUT;
      reboundTimeout = now + REBOUND_TIME;
      pressed = true;
      counter++;
      if (counter == 3) {
        isActive = !isActive;
        counter = 0;
      }
    }
    // Atencion a la interrupcion del pin de presencia del intruso
  } else if (status & BIT(PIN_D8)) {
    if (isActive) {
      led_on();
    }
  }

  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
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
