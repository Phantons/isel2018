#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define TIMEOUT 1000/portTICK_RATE_MS

int PIN_LIGHT;

enum light_state {
  LIGHT_ON,
  LIGHT_OFF
};

portTickType timeout = 0;
volatile bool isPresent = false;

int isSomeoneHere(fsm_t* this) {
  return isPresent;
}

int isSomeoneHereAndTimeout(fsm_t* this) {
  return  xTaskGetTickCount() > timeout && !isPresent;
}

void switchOnTheLight(fsm_t* this) {
  GPIO_OUTPUT_SET(PIN_LIGHT, 0);
  isPresent = false;
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void switchOffTheLight(fsm_t* this) {
  GPIO_OUTPUT_SET(PIN_LIGHT, 1);
}


void isr_gpio() {
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

  // Atencion a la interrupcion de la presencia
  if (status & BIT(PIN_LIGHT)) {
      isPresent = true;
  }

  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

fsm_t*
fsm_new_lights (int pinLight)
{
  static fsm_trans_t tt[] = {
    {LIGHT_OFF, isSomeoneHere , LIGHT_ON, switchOnTheLight},
    {LIGHT_ON,  isSomeoneHereAndTimeout, LIGHT_OFF, switchOffTheLight},
    {-1, NULL, -1, NULL},
  };

    PIN_LIGHT = pinLight;
    return fsm_new (tt);
}
