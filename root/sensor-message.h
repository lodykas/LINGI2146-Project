#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Sensor message management -----------------------------------------*/
struct sensor_message_struct
{
    uint8_t msg;
    rimeaddr_t addr;
    uint8_t value;
};
typedef struct sensor_message_struct sensor_message_t;

union sensor_union
{
    char* c;
    sensor_message_t* st;
};
typedef union sensor_union sensor_u;

sensor_u* create_sensor_message(uint8_t msg, uint8_t ack, uint8_t seqnum, rimeaddr_t addr, uint8_t value);

void send_sensor_message(struct unicast_conn* unicast, const rimeaddr_t* dest, const sensor_u* message);

void free_sensor_message(sensor_u* message);

uint8_t get_msg(sensor_u* message);

uint8_t get_ack(sensor_u* message);

uint8_t get_seqnum(sensor_u* message);
