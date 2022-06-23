#include <stdio.h>

#include "sdc4x.hpp"
#include "scd4x_i2c.h"
#include "pimoroni_i2c.hpp"
#include "sensirion_i2c_hal.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#define MINIMUM_GAP_MS (30 * 1000);

pimoroni::I2C i2c(pimoroni::BOARD::BREAKOUT_GARDEN);

queue_t results_queue;
uint32_t last_time = 0;
bool sensor_active = false;

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

void init_sensor() {
  printf("sensor_init hal...\n");
  sensirion_i2c_hal_init(&i2c);
  printf("sensor_init wake...\n");
  scd4x_wake_up();
  //printf("sensor_init stop...\n");
  //scd4x_stop_periodic_measurement();
  //printf("sensor_init reinit...\n");
  //scd4x_reinit();
  printf("sensor_init queue...\n");
  queue_init(&results_queue, sizeof(Reading), 2);
}

void air_quality_worker() {
  int16_t error;

  printf("air_quality_worker...:\n");

  error = scd4x_start_periodic_measurement();
  if (error) {
    printf("air_quality_worker error calling scd4x_start_periodic_measurement.\n");
  }

  printf("air_quality_worker waiting for data...\n");
  uint16_t _co2;
  int32_t _temperature;
  int32_t _humidity;
  while (true) {
    sensirion_i2c_hal_sleep_usec(5000000);
    uint16_t data_ready = 0;
    error = scd4x_get_data_ready_status(&data_ready);
    if (error) {
      printf("air_quality_worker error calling scd4x_get_data_ready_status.\n");
      break;
    }

    if (!(data_ready & 0x7ff)) {
      printf(".");
      continue;
    }

    error = scd4x_read_measurement(&_co2, &_temperature, &_humidity);
    if (error) {
      printf("air_quality_worker error calling scd4x_get_data_ready_status.\n");
      continue;
    }
    if (_co2 == 0) {
      printf("air_quality_worker: invalid sample.\n");
      continue;
    }

    error = scd4x_stop_periodic_measurement();
    if (error) {
      printf("air_quality_worker error calling scd4x_stop_periodic_measurement.\n");
    }

    break;
  }

  Reading reading = Reading();

  reading.co2 = _co2;
  printf("air_quality_worker CO2: %uppm\n", reading.co2);

  reading.temperature = _temperature / 1000;
  printf("air_quality_worker Temperature: %.0f°C\n", reading.temperature);

  reading.humidity = _humidity / 1000;
  printf("air_quality_worker Humidity: %.0f%%\n", reading.humidity);

  for (int tries=10; tries>0; --tries) {
    if (queue_try_add(&results_queue, &reading)) break;
    sleep_ms(100 * tries);
  }
  sensor_active = false;
}

int start_air_quality_measurement() {
  // only start a measurement if a sensible amount of time has elapsed
  uint32_t now = time();
  if (now - last_time > (10 * 1000) || last_time == 0) {
    printf("start_air_quality_measurement starting air_quality_worker on core1...\n");
    multicore_reset_core1();
    multicore_launch_core1(air_quality_worker);
    sensor_active = true;
    last_time = now;
    printf("start_air_quality_measurement starting air_quality_worker on core1 done.\n");
    return 0;
  }
  printf("start_air_quality_measurement not enough time has elapsed for a new measurement.\n");
  return 1;
}

int get_air_quality_reading(Reading* reading) {
  printf("get_air_quality_reading checking for a new reading...\n");
  if (queue_try_remove(&results_queue, reading)) {
    printf("get_air_quality_reading new reading.\n");
    return 0;
  }
  printf("get_air_quality_reading no data.\n");
  return 1;
}

bool sensor_is_active() {
  return sensor_active;
}
