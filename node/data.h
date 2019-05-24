#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "sensor-message.h"

//down data
#define PERIODIC_SEND 0
#define ON_CHANGE_SEND 1
#define DONT_SEND 2

//up data
#define TEMPERATURE 0
#define HUMIDITY 1

//ack
#define ACK 1

// !! can not exceed 16 (max seqnum is 15)
#define SENDING_QUEUE_SIZE 16

#define DATA_D_MIN 5
#define DATA_D_MAX 10

/* up = toward parent, from children */
struct unicast_conn sensor_up_unicast;

sensor_u* ups[SENDING_QUEUE_SIZE];
uint8_t sending_cursor_up;
uint8_t receiving_cursor_up;

/* down = from parent, to children */
struct unicast_conn sensor_down_unicast;

sensor_u* downs[SENDING_QUEUE_SIZE];
uint8_t sending_cursor_down;
uint8_t receiving_cursor_down;

/**
 * Open both up and down data unicast connections and initialize the timers
 * related.
 */
void data_open();

/**
 * Close both data connections.
 */
void data_close();

/**
 * Function automatically called when an up data message is received
 */
void recv_up(struct unicast_conn *c, const rimeaddr_t *from);

static const struct unicast_callbacks sensor_up_callbacks = {recv_up};

/**
 * Function automatically called by the down timer, send a message from the queue
 * to the parent.
 */
void send_up(void* ptr);

/**
 * Add a message to the sending queue "up"
 */
void add_up(uint8_t message, rimeaddr_t addr, uint8_t value);

/**
 * Remove a message from the sending queue "up"
 */
void ack_up(uint8_t seqnum, uint8_t msg, rimeaddr_t addr);

/* ------------------------------------------------------------------------- */

/**
 * Function automatically called when a down data message is received
 */
void recv_down(struct unicast_conn *c, const rimeaddr_t *from);

static const struct unicast_callbacks sensor_down_callbacks = {recv_down};

/**
 * Function automatically called by the up timer, send a message from the queue
 * to the corresponding child.
 */
void send_down(void* ptr);

/**
 * Add a message to the sending queue "down"
 */
void add_down(uint8_t message, rimeaddr_t addr);

/**
 * Remove a message from the sending queue "down"
 */
void ack_down(uint8_t seqnum, uint8_t msg, rimeaddr_t addr);
