#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define SENSOR_D 63

struct sensor_data_struct {
    uint8_t last_value;
    uint8_t center;
    uint8_t range;
    struct ctimer next_valuet;
};
typedef struct sensor_data_struct sensor_data_t;

sensor_data_t* create_sensor(uint8_t initial_value, uint8_t range);

uint8_t get_data(sensor_data_t* sensor);

void free_sensor(sensor_data_t* sensor);
