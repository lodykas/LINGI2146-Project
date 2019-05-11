#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Broadcast message managment -----------------------------------------*/
struct route_message_struct {
	uint8_t msg;
	rimeaddr_t addr;
};
typedef struct route_message_struct route_message_t;

union route_union {
	char* c;
	route_message_t* st;
};
typedef union route_union route_u;

route_u* create_route_message(uint8_t msg, rimeaddr_t addr);

void send_route_message(struct unicast_conn* unicast, const rimeaddr_t* dest, route_u* message);

void free_route_message(route_u* message);
