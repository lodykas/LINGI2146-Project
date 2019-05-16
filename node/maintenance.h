#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define KEEP_ALIVE 0
#define ROUTE 1
#define ROUTE_ACK 2
#define ROUTE_WITHDRAW 3

#define KEEP_ALIVE_D_MIN 10
#define KEEP_ALIVE_D_MAX 15
#define ONLINE_D_MIN 30
#define ONLINE_D_MAX 45
#define ROUTE_D_MIN 7
#define ROUTE_D_MAX 9

/**
 * Open the maintenance unicast connection an initaite the timers related
 * Should be called a the beginning of the process
 */
void open_maintenance();

/**
 * Close the maintenance connection
 */
void close_maintenance();

/**
 * Maintenance callback, this method is automatically called each time a message
 * is received on this channel
 */
void maintenance_recv(struct unicast_conn *c, const rimeaddr_t *from);

static const struct unicast_callbacks maintenance_callback = {maintenance_recv};

/**
 * Method automaticaly called periodically by the keep_alive timer, send a 
 * keep_alive message to the parent
 */
void send_keep_alive(void* ptr);

/**
 * Method automaticaly called periodically by the route timer, send a route if
 * any of the table should be shared
 */
void send_route(void* ptr);
