#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define LAST_TEMP_VALUES 10
#define SAMPLING_TIME 5000/portTICK_RATE_MS

#define TEMP_THRESHOLD_L 22
#define TEMP_THRESHOLD_M 23
#define TEMP_THRESHOLD_H 24

#define LOW_V 0.2
#define MEDIUM_V 0.5
#define HIGH_V 0.8

#define TIMEOUT 1000/portTICK_RATE_MS

#define TEMP_SENSOR 4

int PORT_BOTTOM_GRIDS;
int PORT_TOP_GRIDS;
int PORT_FANS;

typedef struct temperature_t temperature_t;
struct temperature_t {
  float lastValues[LAST_TEMP_VALUES];
};

temperature_t lastTemperatureValues;

void temperature(temperature_t* lastTemperatureValues) {
  memset(lastTemperatureValues->lastValues, (float)(0.0), sizeof(int)*LAST_TEMP_VALUES);
}

void insertValue(temperature_t* lastTemperatureValues, int value) {
  for (int i = 1; i < LAST_TEMP_VALUES; i++) {
    lastTemperatureValues->lastValues[i] = lastTemperatureValues->lastValues [i - 1];
  }

  lastTemperatureValues->lastValues[0] = value;
}

int getLastOne(temperature_t* lastTemperatureValues) {
  return lastTemperatureValues->lastValues[0];
}

int getSlope(temperature_t* lastTemperatureValues) {
  float sumX = 0.0;
  float sumY = 0.0;
  float xy = 0.0;
  float x2 = 0.0;
  for (int i = 0; i < LAST_TEMP_VALUES; i++) {
    sumY += lastTemperatureValues->lastValues[i];
    sumX += i*SAMPLING_TIME;
    xy += lastTemperatureValues->lastValues[i]*(i);
    x2 += i*i;
  }

  return (xy - (sumX*sumY)/LAST_TEMP_VALUES) /
          (x2 - ((sumX)*(sumX))/LAST_TEMP_VALUES);
}


portTickType timeout;

enum refrigeration_state {
  NO_REFRIGERATION,
  REFRIGERATION_MODE_1,
  REFRIGERATION_MODE_2,
  REFRIGERATION_MODE_3
};


int checkIfItsGoesUp(fsm_t* this) {
  bool isGoingUp = false;

  int slope =  getSlope(&lastTemperatureValues);
  int lastOne = getLastOne(&lastTemperatureValues);


  switch (this->current_state) {
    case NO_REFRIGERATION:
      isGoingUp = slope > HIGH_V ||
        (slope > MEDIUM_V && lastOne > TEMP_THRESHOLD_L) ||
        (slope > LOW_V && lastOne > TEMP_THRESHOLD_M);
        break;
    case REFRIGERATION_MODE_1:
    isGoingUp = slope > HIGH_V ||
      (slope > MEDIUM_V && lastOne > TEMP_THRESHOLD_L) ||
      (slope > LOW_V && lastOne > TEMP_THRESHOLD_M);
        break;
    case REFRIGERATION_MODE_2:
    isGoingUp = slope > HIGH_V ||
      (slope > MEDIUM_V && lastOne > TEMP_THRESHOLD_L) ||
      (slope > LOW_V && lastOne > TEMP_THRESHOLD_M);
        break;
    case REFRIGERATION_MODE_3:
    isGoingUp = slope > HIGH_V ||
      (slope > MEDIUM_V && lastOne > TEMP_THRESHOLD_L) ||
      (slope > LOW_V && lastOne > TEMP_THRESHOLD_M);
        break;
    default:
      break;
  }

  return isGoingUp && xTaskGetTickCount() > timeout;
}

int checkIfItsGoesDown(fsm_t* this) {
  bool isGoingDown = false;

  switch (this->current_state) {
    case NO_REFRIGERATION:
      break;
    case REFRIGERATION_MODE_1:
      isGoingDown = getLastOne(&lastTemperatureValues) < TEMP_THRESHOLD_L;
      break;
    case REFRIGERATION_MODE_2:
      isGoingDown = getLastOne(&lastTemperatureValues) < TEMP_THRESHOLD_M;
      break;
    case REFRIGERATION_MODE_3:
      isGoingDown = getLastOne(&lastTemperatureValues) < TEMP_THRESHOLD_H;
      break;
    default:
      break;
  }

  return isGoingDown && xTaskGetTickCount() > timeout;
}

void openTopGrids(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_TOP_GRIDS, 0);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void closeTopGrids(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_TOP_GRIDS, 1);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void startFans(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_FANS, 0);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void stopFans(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_FANS, 1);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void openBottomGrids(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_BOTTOM_GRIDS, 0);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void closeBottomGrids(fsm_t* this) {
  GPIO_OUTPUT_SET(PORT_BOTTOM_GRIDS, 1);
  timeout = xTaskGetTickCount() + TIMEOUT;
}

void sensorHandler() {

  gpio_pin_intr_state_set(TEMP_SENSOR, GPIO_PIN_INTR_NEGEDGE);

  srand(time(NULL));

  portTickType xLastWakeTime;
  while (true) {
    xLastWakeTime = xTaskGetTickCount();

    // TODO: Get data from sensor.
    int r = rand() % 15 + 15;
    insertValue(&lastTemperatureValues, r);

    vTaskDelayUntil(&xLastWakeTime, SAMPLING_TIME);
  }
}

fsm_t*
fsm_new_temperature (int portTopGrids, int portFans, int portBottomGrids)
{
    static fsm_trans_t tt[] = {
      {NO_REFRIGERATION, checkIfItsGoesUp, REFRIGERATION_MODE_1, openTopGrids},
      {REFRIGERATION_MODE_1, checkIfItsGoesUp, REFRIGERATION_MODE_2, startFans},
      {REFRIGERATION_MODE_2, checkIfItsGoesUp, REFRIGERATION_MODE_3, openBottomGrids},
      {REFRIGERATION_MODE_3, checkIfItsGoesDown, REFRIGERATION_MODE_2, closeBottomGrids},
      {REFRIGERATION_MODE_2, checkIfItsGoesDown, REFRIGERATION_MODE_1, stopFans},
      {REFRIGERATION_MODE_1, checkIfItsGoesDown, NO_REFRIGERATION, closeTopGrids},
      {-1, NULL, -1, NULL},
    };

    timeout = 0;
    PORT_FANS = portFans;
    PORT_BOTTOM_GRIDS = portBottomGrids;
    PORT_TOP_GRIDS = portTopGrids;

    temperature(&lastTemperatureValues);

    return fsm_new (tt);
}
