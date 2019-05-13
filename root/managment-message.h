#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_RETRANSMISSIONS 10

/*----- Broadcast message managment -----------------------------------------*/
struct manage_message_struct {
	uint8_t msg;
	rimeaddr_t addr;
	uint8_t weight;
	uint8_t data;
};
typedef struct manage_message_struct manage_message_t;

union manage_union {
	char* c;
	manage_message_t* st;
};
typedef union manage_union manage_u;

manage_u* create_manage_message(uint8_t msg, rimeaddr_t addr, uint8_t weight, uint8_t data);

void send_manage_message(struct runicast_conn* runicast, rimeaddr_t* dest, manage_u* message);

void free_manage_message(manage_u* message);
