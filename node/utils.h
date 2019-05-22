#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "routing-table.h"

//states
#define OFFLINE 0
#define CONNECTED 1

// maximum weight
#define MAX_WEIGHT 255

struct unicast_conn maintenance_unicast;

struct broadcast_conn discovery_broadcast;

struct ctimer keep_alivet;
struct ctimer onlinet;
struct ctimer routet;

struct ctimer hellot;
struct ctimer welcomet;
struct ctimer root_checkt;

struct ctimer upt;
struct ctimer downt;

uint8_t debug;

// global variables
uint8_t state;
uint8_t weight;
uint8_t sensor_state;
rimeaddr_t parent;

/**
 * This method should be called each time a node choose a new parent
 */
void choose_parent(rimeaddr_t addr, uint8_t w);

/**
 * This method is automatically called by the online timer
 */
void disconnect(void* ptr);

/**
 * This method should be called each time we receive a message from our parent
 * and the weight shared by the parent should be given in argument
 */
void parent_refresh(uint8_t w);

/**
 * This method take two bounds and return a value between those two (converted 
 * in clocks ticks), the address of the node will influence on this value
 */
unsigned int delay(unsigned int min, unsigned int max);
