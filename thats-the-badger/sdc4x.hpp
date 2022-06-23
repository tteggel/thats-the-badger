#include "state.hpp"

void init_sensor();
int start_air_quality_measurement();
int get_air_quality_reading(Reading* reading);
bool sensor_is_active();
