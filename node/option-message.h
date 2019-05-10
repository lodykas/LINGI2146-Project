#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_RETRANSMISSIONS 4

/*----- Broadcast message managment -----------------------------------------*/
struct option_message_struct {
	uint8_t msg;
	uint8_t value;
};
typedef struct option_message_struct option_message_t;

union option_union {
	char* c;
	option_message_t* st;
};
typedef union option_union option_u;

option_u* create_option_message(uint8_t msg, uint8_t value);

void send_option_message(struct runicast_conn* runicast, rimeaddr_t* dest, option_u* message);

void free_option_message(option_u* message);
