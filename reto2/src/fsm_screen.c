#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "arkanoPi.h"

portTickType timeout;

int checkTimeout() {
  return true;
}

void refresh(fsm_t* this) {
  // refresh
  ActualizaPantalla(&juego.arkanoPi);
}

fsm_t*
fsm_new_screen ()
{
    static fsm_trans_t tt[] = {
      {1, checkTimeout, 1, refresh},
      {-1, NULL, -1, NULL},
    };

    timeout = xTaskGetTickCount();

    InicializaArkanoPi(&juego.arkanoPi);

    return fsm_new (tt);
}
