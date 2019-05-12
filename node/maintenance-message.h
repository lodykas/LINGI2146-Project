#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Broadcast message managment -----------------------------------------*/
struct maintenance_message_struct {
	uint8_t msg;
	rimeaddr_t addr;
	uint8_t weight;
};
typedef struct maintenance_message_struct maintenance_message_t;

union maintenance_union {
	char* c;
	maintenance_message_t* st;
};
typedef union maintenance_union maintenance_u;

maintenance_u* create_maintenance_message(uint8_t msg, rimeaddr_t addr, uint8_t weight);

void send_maintenance_message(struct unicast_conn* unicast, const rimeaddr_t* dest, maintenance_u* message);

void free_maintenance_message(maintenance_u* message);
