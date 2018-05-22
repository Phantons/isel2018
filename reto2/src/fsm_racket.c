#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#include "arkanoPi.h"

#define REBOUND_TIME 500/portTICK_RATE_MS

int BUTTON_UP;
int BUTTON_DOWN;

portTickType reboundTimeout;
volatile bool pressedUp = false;
volatile bool pressedDown = false;

int isPressedUp(fsm_t* this) {
  return pressedUp;
}

int isPressedDown(fsm_t* this) {
  return pressedDown;
}

void goUp() {
  pressedUp = false;

  if(juego.arkanoPi.raqueta.x < 9){
    juego.arkanoPi.raqueta.x += 1;
  }
}

void goDown() {
  pressedDown = false;

	if(juego.arkanoPi.raqueta.x > 0){
		juego.arkanoPi.raqueta.x -= 1;
	}
}

void isr_gpio() {
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

  long now = xTaskGetTickCount();
  // Atencion a la interrupcion del boton up
  if (status & BIT(BUTTON_UP)) {
    if (now > reboundTimeout) {
      reboundTimeout = now + REBOUND_TIME;
      pressedUp = true;
    }
  }
  // Atencion a la interrupcion del boton up
  else if (status & BIT(BUTTON_DOWN)) {
    if (now > reboundTimeout) {
      reboundTimeout = now + REBOUND_TIME;
      pressedDown = true;
    }
  }

  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

fsm_t*
fsm_new_racket (int buttonUp, int buttonDown)
{
  static fsm_trans_t tt[] = {
    {1, isPressedUp , 1, goUp},
    {1,  isPressedDown, 1, goDown},
    {-1, NULL, -1, NULL},
  };

    BUTTON_UP = buttonUp;
    BUTTON_DOWN = buttonDown;
    return fsm_new (tt);
}
