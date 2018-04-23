#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define LIGHT_D3 0
#define PORT_TOP_GRIDS 1
#define PORT_BOTTOM_GRIDS 2
#define PORT_FANS 3

#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TIME 300/portTICK_RATE_MS
#define TIMEOUT 1000/portTICK_RATE_MS

#define ETS_GPIO_INTR_DISABLE() \
  _xt_isr_mask(1 << ETS_GPIO_INUM)
#define ETS_GPIO_INTR_ENABLE() \
  _xt_isr_unmask(1 << ETS_GPIO_INUM)



void isr_gpio();
fsm_t* fsm_new_lights(int);


fsm_t* fsm_new_temperature(int portTopGrids, int portFans, int portBottomGrids);
void sensorHandler();

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
    fsm_t* light_fsm = fsm_new_lights(LIGHT_D3);

    fsm_t* refrigeration_fsm =
      fsm_new_temperature(PORT_TOP_GRIDS, PORT_FANS, PORT_BOTTOM_GRIDS);

    gpio_intr_handler_register((void*)isr_gpio, NULL);

    gpio_pin_intr_state_set(LIGHT_D3, GPIO_PIN_INTR_NEGEDGE);

    gpio_pin_intr_state_set(PORT_TOP_GRIDS, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(PORT_BOTTOM_GRIDS, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(PORT_FANS, GPIO_PIN_INTR_NEGEDGE);

    ETS_GPIO_INTR_ENABLE();

    portTickType xLastWakeTime;
    while (true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(light_fsm);
      fsm_fire(refrigeration_fsm);

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

  xTaskCreate(&run, "startup", 2048, NULL, 2, NULL);
  xTaskCreate(&sensorHandler, "sensorHandler", 2048, NULL, 1, NULL);
}
