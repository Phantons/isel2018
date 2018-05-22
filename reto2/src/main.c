#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define BUTTON_UP_D3 0
#define BUTTON_DOWN_D8 15

#define ETS_GPIO_INTR_DISABLE() \
  _xt_isr_mask(1 << ETS_GPIO_INUM)
#define ETS_GPIO_INTR_ENABLE() \
  _xt_isr_unmask(1 << ETS_GPIO_INUM)


#define PERIOD_TICK 100/portTICK_RATE_MS

void isr_gpio();
fsm_t* fsm_new_racket(int, int);
fsm_t* fsm_new_ball();
fsm_t* fsm_new_screen();


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

void run(void* ignore)
{
    fsm_t* screen_fsm = fsm_new_screen();

    fsm_t* ball_fsm = fsm_new_ball();

    fsm_t* racket_fsm =
      fsm_new_racket(BUTTON_UP_D3, BUTTON_DOWN_D8);

    gpio_intr_handler_register((void*)isr_gpio, NULL);

    gpio_pin_intr_state_set(BUTTON_UP_D3, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(BUTTON_DOWN_D8, GPIO_PIN_INTR_POSEDGE);

    ETS_GPIO_INTR_ENABLE();

    portTickType xLastWakeTime = xTaskGetTickCount();


    portTickType t1;
    portTickType t2;
    portTickType t3;
    portTickType t4;

    portTickType maxBallTime;
    portTickType maxRacketTime;
    portTickType maxScreenTime;

    int i = 0;
    while (true) {
      t1 = xTaskGetTickCount();
      fsm_fire(screen_fsm);
      t2 = xTaskGetTickCount();
      fsm_fire(racket_fsm);
      t3 = xTaskGetTickCount();
      fsm_fire(ball_fsm);
      t4 = xTaskGetTickCount();

      if ((t2 - t1) > maxScreenTime)
        maxScreenTime = t2 - t1;

      if ((t3 - t2) > maxRacketTime)
        maxRacketTime = t3 - t2;

      if ((t4 - t3) > maxBallTime)
        maxBallTime = t4 - t3;



      if (i == 200) {
        int maxBallTimeF = maxBallTime * portTICK_RATE_MS;
        int maxRacketTimeF = maxRacketTime * portTICK_RATE_MS;
        int maxScreenTimeF = maxScreenTime * portTICK_RATE_MS;

        printf("Ball time [%d] Racket Time [%d Screen time [%d]\n", maxBallTimeF, maxRacketTimeF, maxScreenTimeF);
      } else
        i++;

      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
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
