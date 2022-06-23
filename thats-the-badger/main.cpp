#include <cstdio>
#include <string>
#include <sstream>
#include <iomanip>

#include "badger2040.hpp"

#include "sdc4x.hpp"

#include "badge.hpp"

pimoroni::Badger2040 badger;

State state = State();
bool state_dirty = false;
Screen painted_screen = None;

Reading reading = Reading();

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

void draw_badge() {
  badger.pen(15);
  badger.clear();
  badger.image(badge_bitmap);
}

void draw_right_text(std::string text, float font_size, int right, int top) {
  int width = badger.measure_text(text, font_size);
  badger.pen(15);
  badger.rectangle(right - width, (top-16), width, 28);
  badger.pen(0);
  badger.text(text, right - width, top, font_size);
}

void draw_badge_air_data() {
  badger.pen(0);
  badger.font("sans");
  badger.thickness(2);

  std::string f = to_str(ctof(reading.temperature));
  draw_right_text(f, 0.7f, 41, 113);

  std::string c = to_str(reading.temperature);
  draw_right_text(c, 0.7f, 96, 113);

  std::string h = to_str(reading.humidity);
  draw_right_text(h, 0.7f, 158 , 113);

  std::string p = to_str(reading.co2);
  draw_right_text(p, 0.7f, 240, 113);
};

void draw_aqm() {
  badger.pen(15);
  badger.clear();

  badger.pen(0);
  badger.font("serif");
  badger.thickness(2);
  badger.text(to_str(reading.co2) + "ppm", 90, 10, 0.80f);
  badger.text(to_str(reading.temperature) + "°C / " + to_str(ctof(reading.temperature)) + "°F", 90, 32, 0.80f);
  badger.text(to_str(reading.humidity) + "%", 90, 54, 0.80f);
}

int main() {
  badger.init();
  stdio_init_all();
  badger.update_speed(0);
  sleep_ms(1000);
  printf("init... done.\n");

  printf("get_state...\n");
  get_state(&state);
  printf("get_state {magic: %X, screen: %d, reading_index: %d}\n", state.magic,
         state.current_screen, state.reading_index);
  reading = state.readings[state.reading_index];
  printf("get_state done.\n");

  if (badger.pressed_to_wake(badger.A)) {
    printf("state.current_screen = Badge...\n");
    state.current_screen = Badge;
    state_dirty = true;
    printf("state.current_screen = Badger done.\n");
  }

  if (badger.pressed_to_wake(badger.B)) {
    printf("state.current_screen = AirQuality...\n");
    state.current_screen = AirQuality;
    state_dirty = true;
    printf("state.current_screen = AirQuality done.\n");
  }

  printf("sensor_init...\n");
  init_sensor();
  printf("sensor_init done.\n");

  while (true) {
    printf("main_loop...\n");

    if (badger.pressed(badger.A)) {
      printf("main_loop state.current_screen = Badge...\n");
      state.current_screen = Badge;
      state_dirty = true;
      printf("main_loop state.current_screen = Badge done.\n");
    }

    if (badger.pressed(badger.B)) {
      printf("main_loop state.current_screen = AirQuality...\n");
      state.current_screen = AirQuality;
      state_dirty = true;
      printf("main_loop state.current_screen = AirQuality done.\n");
    }

    if (badger.pressed(badger.C)) {
      printf("main_loop halt...\n");
      wait_for_idle();
      badger.halt();
      printf("main_loop SHOULD NOT GET HERE.\n");
      break;
    }

    printf("start_air_quality_measurement...\n");
    if (start_air_quality_measurement() == 0) {
      printf("start_air_quality_measurement done.\n");
    } else {
      printf("start_air_quality_measurement not started.\n");
    }

    if (state.current_screen == AirQuality && painted_screen != AirQuality) {
      printf("main_loop draw_aqm...\n");
      draw_aqm();
      printf("main_loop draw_aqm done.\n");
    } else if (state.current_screen == Badge && painted_screen != Badge) {
      printf("main_loop draw_badge...\n");
      draw_badge();
      draw_badge_air_data();
      printf("main_loop draw_badge done.\n");
    }

    if (painted_screen != state.current_screen) {
      printf("main_loop update...\n");
      badger.update();
      painted_screen = state.current_screen;
      printf("main_loop update done.\n");
    }

    if (get_air_quality_reading(&reading) == 0) {
      state.reading_index = state.reading_index + 1 % READING_SAMPLE_COUNT;
      state.readings[state.reading_index] = reading;
      state_dirty = true;
      if (state.current_screen == Badge) {
        draw_badge_air_data();
        badger.partial_update(0, 128-32, 296, 32);
      }
    }

    if (state_dirty) {
      printf("main_loop store_state...\n");
      printf("main_loop store_state {magic: %X, screen: %d, reading_index: %d}\n", state.magic,
             state.current_screen, state.reading_index);
      store_state(&state);
      state_dirty = false;
      printf("main_loop store_state done.\n");
    }

    if (sensor_is_active()) {
      sleep_ms(1000);
      badger.update_button_states();
    } else {
      printf("main_loop wait_for_press...\n");
      wait_for_idle();
      badger.wait_for_press();
      printf("main_loop wait_for_press done.\n");
    }
  }
}
