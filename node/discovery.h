#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define HELLO 0
#define WELCOME 1
#define ROOT_CHECK 2

#define HELLO_D_MIN 5
#define HELLO_D_MAX 7
#define WELCOME_D_MIN 2
#define WELCOME_D_MAX 4
#define ROOT_CHECK_D_MIN 2
#define ROOT_CHECK_D_MAX 4

/**
 * Open the discovery broadcast channel an initialize the timers related
 */
void open_discovery();

/**
 * Close the discovery channel
 */
void close_discovery();

/**
 * Function automatically called when a broadcast message is received on the
 * discovery channel. Should process the message.
 */
void discovery_recv(struct broadcast_conn *c, const rimeaddr_t *from);

static const struct broadcast_callbacks discovery_callback = {discovery_recv};

/**
 * Function automatically called by the hello timer when the node is offline.
 */
void send_hello(void* ptr);

/**
 * Function automatically called by the welcome timer when a welcome needs to be
 * send
 */
void send_welcome(void* ptr);

/**
 * Function automatically called by the root_check timer when a root_check needs
 * to be send.
 */
void send_root_check(void* ptr);
