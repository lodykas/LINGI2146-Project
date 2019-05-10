#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Broadcast message managment -----------------------------------------*/
struct discovery_struct {
	uint8_t msg;
	uint8_t weight;
};
typedef struct discovery_struct discovery_struct_t;

union discovery_union {
	char* c;
	discovery_struct_t* st;
};
typedef union discovery_union discovery_t;

discovery_t* create_broadcast_message(uint8_t msg, uint8_t weight);

void send_broadcast_message(struct broadcast_conn* broadcast, discovery_t* message);

void free_broadcast_message(discovery_t* message);
