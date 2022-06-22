#include <stdio.h>

#include <string>
#include <sstream>
#include <iomanip>

#include "badger2040.hpp"
#include "common/pimoroni_common.hpp"
#include "pico/platform.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "scd4x_i2c.h"
#include "pimoroni_i2c.hpp"
#include "sensirion_i2c_hal.h"

#include "state.hpp"

using namespace pimoroni;

Badger2040 badger;
I2C i2c(BOARD::BREAKOUT_GARDEN);
State state = State();

std::string to_str(float f) {
  std::ostringstream os;
  os << std::fixed << std::setprecision(0) << f;
  return os.str();
}

float ctof(float c) {
  return (c * 9 / 5) + 32;
}

void wait_for_idle() {
  if (!badger.is_busy()) return;
  while (badger.is_busy()) sleep_ms(10);
}

int read_air_quality(uint16_t *co2, float *temperature,
                     float *humidity) {
  int16_t error;

  printf("    starting read_air_quality:\n");

  error = scd4x_start_periodic_measurement();
  if (error) {
    printf("    error calling scd4x_start_periodic_measurement.\n");
  }

  printf("    waiting for data:\n");
  uint16_t _co2;
  int32_t _temperature;
  int32_t _humidity;
  while (true) {
    sensirion_i2c_hal_sleep_usec(5000000);
    uint16_t data_ready = 0;
    error = scd4x_get_data_ready_status(&data_ready);
    if (error) {
      printf("      error calling scd4x_get_data_ready_status.\n");
      return 2;
    }

    if (!(data_ready & 0x7ff)) {
      printf(".");
      continue;
    }

    error = scd4x_read_measurement(&_co2, &_temperature, &_humidity);
    if (error) {
      printf("      error calling scd4x_get_data_ready_status.\n");
      continue;
    }
    if (co2 == nullptr) {
      printf("      invalid sample.\n");
      continue;
    }

    error = scd4x_stop_periodic_measurement();
    if (error) {
      printf("      error calling scd4x_stop_periodic_measurement.\n");
    }

    break;
  }

  *co2 = _co2;
  printf("      CO2: %u\n", *co2);

  *temperature = _temperature / 1000;
  printf("      Temperature: %.0f °C\n", *temperature);

  *humidity = _humidity / 1000;
  printf("      Humidity: %.0f %%\n", *humidity);

  return 0;
}

void draw_badge() {
  badger.pen(0);
  badger.font("serif");
  badger.thickness(2);
  badger.text("Badgeer", 90, 10, 1.0f);
}

void draw_aqm() {
  uint16_t co2;
  float temperature;
  float humidity;
  read_air_quality(&co2, &temperature, &humidity);

  badger.pen(0);
  badger.font("serif");
  badger.thickness(2);
  badger.text(to_str(co2) + "ppm", 90, 10, 0.80f);
  badger.text(to_str(temperature) + "°C / " + to_str(ctof(temperature)) + "°F", 90, 32, 0.80f);
  badger.text(to_str(humidity) + "%", 90, 54, 0.80f);
}

int main() {
  badger.init();
  stdio_init_all();
  badger.update_speed(1);
  sleep_ms(1000);
  printf("init: done.\n");

  printf("get_state: ");
  get_state(&state);
  printf("{magic: %X, screen: %d, co2_index: %d,temp_index: %d, humidity_index: %d} ", state.magic,
         state.current_screen, state.co2_index, state.temp_index, state.humidity_index);
  printf("done.\n");

  if (badger.pressed_to_wake(badger.A)) {
    printf("  state.current_screen = Badge: ");
    state.current_screen = Badge;
    printf("done.\n");
  }

  if (badger.pressed_to_wake(badger.B)) {
    printf("  state.current_screen = AirQuality: ");
    state.current_screen = AirQuality;
    printf("done.\n");
  }

  printf("sensor_init: ");
  sensirion_i2c_hal_init(&i2c);
  scd4x_wake_up();
  scd4x_stop_periodic_measurement();
  scd4x_reinit();
  printf("done.\n");

  while (true) {
    printf("main_loop:\n");

    printf("  clear: ");
    badger.pen(15);
    badger.clear();
    printf("done.\n");

    if (state.current_screen == AirQuality) {
      printf("  draw_aqm:\n");
      draw_aqm();
      printf("  draw_aqm done.\n");
    } else if (state.current_screen == Badge) {
      printf("  draw_badge:\n");
      draw_badge();
      printf("  draw_badge done.\n");
    }

    printf("  update: ");
    badger.update();
    printf("done.\n");

    printf("  wait_for_press: ");
    badger.wait_for_press();
    printf("done.\n");

    if (badger.pressed(badger.A)) {
      printf("  state.current_screen = Badge: ");
      state.current_screen = Badge;
      printf("done.\n");
    }

    if (badger.pressed(badger.B)) {
      printf("  state.current_screen = AirQuality: ");
      state.current_screen = AirQuality;
      printf("done.\n");
    }

    printf("  store_state: ");
    printf("{magic: %x, screen: %d, co2_index: %d,temp_index: %d, humidity_index: %d} ", state.magic,
           state.current_screen, state.co2_index, state.temp_index, state.humidity_index);
    store_state(&state);
    printf("done.\n");

    if (badger.pressed(badger.C)) {
      printf("  halt: ");
      badger.halt();
      printf("NOT DONE.\n");
    }
  }
}
