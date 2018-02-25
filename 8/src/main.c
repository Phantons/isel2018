#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define BUTTON_D3 0
#define PIN_D8 15
#define LED 2
#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TIME 300/portTICK_RATE_MS
#define TIMEOUT 1000/portTICK_RATE_MS
#define CODE_LENGTH 3
#define ETS_GPIO_INTR_DISABLE() \
  _xt_isr_mask(1 << ETS_GPIO_INUM)
#define ETS_GPIO_INTR_ENABLE() \
  _xt_isr_unmask(1 << ETS_GPIO_INUM)



enum alarm_state {
  ALARM_OFF,
  ALARM_ON,
};

enum check_code_state {
  CHECK_CODE
};

portTickType reboundTimeout = 0;
portTickType timeout = 0;
volatile bool isPresent = false;
volatile bool pressed = false;
bool isPlacingCode = false;
bool codeIsCorrect = false;
int code_index = 0;
int code_inserted[CODE_LENGTH];
const int CODE[] = {1, 2, 3};

int checkIfPressed(fsm_t*);
int checkTimeout(fsm_t*);
int checkCorrectCode(fsm_t*);
int checkIncorrectCode(fsm_t*);
int isSomeoneHere(fsm_t*);
int checkCorrectCodeFlag(fsm_t*);

void led_on(fsm_t*);
void led_off(fsm_t*);
void updateCode(fsm_t*);
void clearFlags(fsm_t*);
void setCorrectCodeFlag(fsm_t*);
void nextIndex(fsm_t*);

void isr_gpio();

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

struct fsm_trans_t trans_alarm[] = {
  {ALARM_OFF, checkCorrectCodeFlag, ALARM_ON, clearFlags},
  {ALARM_ON, checkCorrectCodeFlag, ALARM_OFF, led_off},
  {ALARM_ON, isSomeoneHere, ALARM_ON, led_on},
  {-1, NULL, -1, NULL},
};

struct fsm_trans_t check_code[] = {
  {CHECK_CODE, checkIfPressed, CHECK_CODE, updateCode},
  {CHECK_CODE, checkTimeout, CHECK_CODE, nextIndex},
  {CHECK_CODE, checkCorrectCode, CHECK_CODE, setCorrectCodeFlag},
  {CHECK_CODE, checkIncorrectCode, CHECK_CODE, clearFlags},
};

void run(void* ignore)
{
    fsm_t* alarm_fsm = fsm_new(trans_alarm);
    led_off(alarm_fsm);

    fsm_t* check_code_fsm = fsm_new(check_code);
    clearFlags(check_code_fsm);

    gpio_intr_handler_register((void*)isr_gpio, NULL);
    gpio_pin_intr_state_set(BUTTON_D3, GPIO_PIN_INTR_NEGEDGE);
    gpio_pin_intr_state_set(PIN_D8, GPIO_PIN_INTR_POSEDGE);

    ETS_GPIO_INTR_ENABLE();

    portTickType xLastWakeTime;
    while (true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(check_code_fsm);
      fsm_fire(alarm_fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

int checkIfPressed(fsm_t* this) {
  if (code_index > 2)
    return 0;
  if (code_inserted[code_index] > 10)
    return 0;

  return pressed;
}

int checkTimeout(fsm_t* this) {
  return xTaskGetTickCount() > timeout && isPlacingCode;
}

int checkCorrectCodeFlag(fsm_t* this) {
  return codeIsCorrect;
}

int isSomeoneHere(fsm_t* this) {
  return isPresent;
}

int checkCorrectCode(fsm_t* this) {
  return code_index == 3 && memcmp(code_inserted, CODE, sizeof(code_inserted)) == 0;
}

int checkIncorrectCode(fsm_t* this) {
  return code_index == 3 && memcmp(code_inserted, CODE, sizeof(code_inserted)) != 0;
}

void led_on (fsm_t* this) {
  GPIO_OUTPUT_SET(LED, 0);
  isPresent = false;
}

void led_off (fsm_t* this) {
  GPIO_OUTPUT_SET(LED, 1);
  clearFlags(this);
}

void updateCode(fsm_t* this) {
  isPlacingCode = true;
  code_inserted[code_index]++;
  timeout = xTaskGetTickCount() + TIMEOUT;
  pressed = false;
}

void setCorrectCodeFlag(fsm_t* this) {
  codeIsCorrect = true;
}

void nextIndex(fsm_t* this) {
  code_index++;
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void clearFlags(fsm_t* this) {
  pressed = false;
  isPresent = false;
  codeIsCorrect = false;
  code_index = 0;
  isPlacingCode = false;
  memset(code_inserted, 0, sizeof(code_inserted));
}

void isr_gpio() {
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  static bool isActive = false;
  long now = xTaskGetTickCount();
  // Atencion a la interrupcion del boton de armar/desarmar
  if (status & BIT(BUTTON_D3)) {
    if (now > reboundTimeout) {
      reboundTimeout = now + REBOUND_TIME;
      pressed = true;
    }
    // Atencion a la interrupcion del pin de presencia del intruso
  } else if (status & BIT(PIN_D8)) {
    isPresent = true;
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
