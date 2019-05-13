#include "sensor-data.h"

void generate_next_value(void* ptr) {
    sensor_data_t* sensor = (sensor_data_t*) ptr;
    uint8_t last_value = sensor->last_value;
    uint8_t step = random_rand() % 3;
    uint8_t plus = random_rand() % 2;
    if (plus && (last_value + step < sensor->center + sensor->range)) {
        sensor->last_value = last_value + step;
    } else if (last_value - step > sensor->center - sensor->range) {
        sensor->last_value = last_value - step;
    } else {
        sensor->last_value = sensor->center;
    }
    
    ctimer_restart(&(sensor->next_valuet));
}

sensor_data_t* create_sensor(uint8_t initial_value, uint8_t range) {
    sensor_data_t* sensor = (sensor_data_t*) malloc(sizeof(sensor_data_t));
    sensor->last_value = initial_value;
    sensor->center = initial_value;
    sensor->range = range;
    ctimer_set(&(sensor->next_valuet), SENSOR_D * CLOCK_SECOND, generate_next_value, sensor);
    return sensor;
}

uint8_t get_data(sensor_data_t* sensor) {
    return sensor->last_value;
}

void free_sensor(sensor_data_t* sensor) {
    if (sensor != NULL) {
        ctimer_stop(&(sensor->next_valuet));
        free(sensor);
    }
}
