#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Broadcast message managment -----------------------------------------*/
struct routing_message_struct {
	uint8_t msg;
	rimeaddr_t addr;
};
typedef struct routing_message_struct routing_message_t;

union routing_union {
	char* c;
	routing_message_t* st;
};
typedef union routing_union routing_u;

routing_u* create_unicast_message(uint8_t msg, rimeaddr_t addr);

void send_unicast_message(struct unicast_conn* unicast, rimeaddr_t* dest, routing_u* message);

void free_unicast_message(routing_u* message);
