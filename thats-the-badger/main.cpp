#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <limits>
#include <array>

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
  badger.pen(0);
  badger.text(text, right - width, top, font_size);
}

void draw_badge_air_data() {
  badger.pen(0);
  badger.font("sans");
  badger.thickness(2);

  std::string f = to_str(ctof(reading.temperature));
  badger.pen(15);
  badger.rectangle(2, 100, 37, 27);
  draw_right_text(f, 0.7f, 41, 113);

  std::string c = to_str(reading.temperature);
  badger.pen(15);
  badger.rectangle(59, 100, 94-59, 27);
  draw_right_text(c, 0.7f, 96, 113);

  std::string h = to_str(reading.humidity);
  badger.pen(15);
  badger.rectangle(116, 100, 157-116, 27);
  draw_right_text(h, 0.7f, 158 , 113);

  std::string p = to_str(reading.co2);
  badger.pen(15);
  badger.rectangle(173, 100, 239-173, 27);
  draw_right_text(p, 0.7f, 240, 113);
};

float lerp(float from, float to, float rel) {
  return ((1 - rel) * from) + (rel * to);
}

float invlerp(float from, float to, float value) {
  return (value - from) / (to - from);
}

float remap(float orig_from, float orig_to, float target_from, float target_to, float value){
  float rel = invlerp(orig_from, orig_to, value);
  return lerp(target_from, target_to, rel);
}

float min(std::array<float, READING_SAMPLE_COUNT> array) {
  float min = std::numeric_limits<float>().max();
  for (int i=0; i<READING_SAMPLE_COUNT; ++i) {
    if (array[i] < min && array[i] != 0) min = array[i];
  }
  return min;
}

float max(std::array<float, READING_SAMPLE_COUNT> array) {
  float max = std::numeric_limits<float>().min();
  for (int i=0; i<READING_SAMPLE_COUNT; ++i) {
    if (array[i] > max && array[i] != 0) max = array[i];
  }
  return max;
}

void draw_line_chart(std::string name, std::string unit, std::array<float, READING_SAMPLE_COUNT> data, int xmin, int xmax, int ymin, int ymax) {
  float data_min = min(data);
  float data_max = *std::max_element(data.begin(), data.end());

  badger.font("bitmap4");
  badger.font("bitmap8");
  badger.font("bitmap4");
  badger.thickness(1);
  draw_right_text(to_str(data_max), 1, 260, ymin);
  draw_right_text(to_str(data_min), 1, 260, ymax - 4);

  badger.font("bitmap8");
  badger.thickness(2);
  draw_right_text(name, 1, 290, ymin);

  badger.font("bitmap16_outline");
  badger.thickness(1);
  draw_right_text(to_str(data[READING_SAMPLE_COUNT-1]) + unit, 2, 290, ymin + 10);

  if(data_max == data_min) return;

  printf("min: %f, max: %f\n", data_min, data_max);

  for (int index = 1; index < READING_SAMPLE_COUNT; ++index) {
    if (data[index - 1] == 0 || data[index] == 0) continue;

    int x0 = remap(0, READING_SAMPLE_COUNT, xmin, xmax, index - 1);
    int y0 = remap(data_min, data_max, ymax, ymin, data[index - 1]);
    int x1 = remap(0, READING_SAMPLE_COUNT, xmin, xmax, index);
    int y1 = remap(data_min, data_max, ymax, ymin, data[index]);

    //printf("(%u, %u) -> (%u, %u)\n", x0, y0, x1, y1);

    badger.line(x0, y0, x1, y1);
  }
}

void draw_aqm() {
  badger.pen(15);
  badger.clear();
  wait_for_idle();

  badger.pen(0);
  badger.thickness(1);

  std::array<float, READING_SAMPLE_COUNT> temps;
  std::array<float, READING_SAMPLE_COUNT> humids;
  std::array<float, READING_SAMPLE_COUNT> co2s;

  std::transform(state.readings.begin(), state.readings.end(), temps.begin(), [](Reading r) { return r.temperature; });
  std::transform(state.readings.begin(), state.readings.end(), humids.begin(), [](Reading r) { return r.humidity; });
  std::transform(state.readings.begin(), state.readings.end(), co2s.begin(), [](Reading r) { return r.co2; });

  std::rotate(temps.begin(), temps.begin() + state.reading_index + 1, temps.end());
  std::rotate(humids.begin(), humids.begin() + state.reading_index + 1, humids.end());
  std::rotate(co2s.begin(), co2s.begin() + state.reading_index + 1, co2s.end());

#define CHART_MARGIN 6
#define SECTION_HEIGHT 42
  draw_line_chart("TEMP", "°C", temps,2, 225, CHART_MARGIN, SECTION_HEIGHT - 0.5*CHART_MARGIN);
  draw_line_chart("RH", "%", humids,2, 225, SECTION_HEIGHT + 0.5*CHART_MARGIN,  2*SECTION_HEIGHT - 0.5*CHART_MARGIN);
  draw_line_chart("CO2", "ppm", co2s,2, 225, 2*SECTION_HEIGHT + 0.5*CHART_MARGIN, 3*SECTION_HEIGHT - CHART_MARGIN);
}

int main() {
  badger.init();
  stdio_init_all();
  badger.update_speed(1);
  //sleep_ms(1000);
  printf("init... done.\n");

  printf("get_state...\n");
  get_state(&state);
  printf("get_state {magic: %X, screen: %d, reading_index: %d}\n", state.magic,
         state.current_screen, state.reading_index);
  printf("get_state temp %f\n", state.readings[state.reading_index].temperature);

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
      state.reading_index = (state.reading_index + 1) % READING_SAMPLE_COUNT;
      memcpy(&state.readings[state.reading_index], &reading, sizeof(Reading));
      state_dirty = true;
      if (state.current_screen == Badge) {
        draw_badge_air_data();
        badger.partial_update(0, 128-32, 296, 32);
      }
      if (state.current_screen == AirQuality){
        draw_aqm();
        badger.update();
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
      sleep_ms(50);
      badger.update_button_states();
    } else {
      printf("main_loop halt...\n");
      wait_for_idle();
      badger.halt();
      printf("main_loop halt done.\n");
    }
  }
}
