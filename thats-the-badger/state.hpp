#include "pico/platform.h"

#define READING_SAMPLE_COUNT 32

enum Screen : uint8_t {
    None,
    Badge,
    AirQuality
};

class Reading {
public:
    Reading() = default;
    uint16_t co2 = 0;
    float temperature = 0;
    float humidity = 0;
};

class State {
public:
    State() = default;

    uint16_t magic = 0x6023;

    Screen current_screen = Badge;

    Reading readings[READING_SAMPLE_COUNT] = {};
    uint8_t reading_index = 0;
};

void store_state(const State *data);

void get_state(State *state);
