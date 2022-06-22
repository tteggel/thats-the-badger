#include "pico/platform.h"

enum Screen : uint8_t {
    Badge,
    AirQuality
};

struct State {
public:
    State() = default;

    uint16_t magic = 0x6022;

    Screen current_screen = Badge;

    float co2s[32] = {};
    uint8_t co2_index = 0;

    float temps[32] = {};
    uint8_t temp_index = 0;

    float humidities[32] = {};
    uint8_t humidity_index = 0;
};

void store_state(const State *data);

void get_state(State *state);
