#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_RETRANSMISSIONS 10

/*----- Broadcast message managment -----------------------------------------*/
struct data_message_struct {
	uint8_t msg;
	rimeaddr_t addr;
	uint8_t value;
};
typedef struct data_message_struct data_message_t;

union data_union {
	char* c;
	data_message_t* st;
};
typedef union data_union data_u;

data_u* create_data_message(uint8_t msg, rimeaddr_t addr, uint8_t value);

void send_data_message(struct runicast_conn* runicast, rimeaddr_t* dest, data_u* message);

void free_data_message(data_u* message);
