/**
 * \file
 *		 Testing the broadcast layer in Rime
 * \author
 *		 Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>

//messages
#define WELCOME 1
#define CHOOSE 2

//timeout
#define RESEND_WELCOME 2

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);

/*----- Message managment ---------------------------------------------------*/
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

discovery_t* create_message(uint8_t msg, uint8_t weight) {
	discovery_struct_t* st = (discovery_struct_t*) malloc(sizeof(discovery_struct_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->weight = weight;
	
	discovery_t* message = (discovery_t*) malloc(sizeof(discovery_t));
	if (message == NULL) return NULL;
	message->st = st;
	
	return message;
}

void send_message(struct broadcast_conn* broadcast, discovery_t* message) {
	packetbuf_copyfrom(message->c, sizeof(discovery_struct_t));
	broadcast_send(broadcast);
}

void free_message(discovery_t* message) {
	if (message->st != NULL) free(message->st);
	free(message);
}
/*---------------------------------------------------------------------------*/

// global variables
static struct broadcast_conn broadcast;
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from) {
	discovery_t message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];
	
	switch(message.st->msg) {
		case CHOOSE:
			printf("CHOOSE message received from %d.%d: '%d'\n", u0, u1, w);

			break;
		default:
			printf("UNKOWN message received from %d.%d: '%d'\n", u0, u1, 
				message.st->msg);
			
			break;
	}
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from) {}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_broadcast_process, ev, data)
{
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	unicast_open(&uc, 146, &unicast_callbacks);
	
	static struct etimer welcomet;

	while(1) {
	
        /* Delay 2-4 seconds */
        etimer_set(&welcomet, CLOCK_SECOND * RESEND_WELCOME + random_rand() % (CLOCK_SECOND * RESEND_WELCOME));

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&welcomet));
        
	    discovery_t* welcome = create_message(WELCOME, 0);
	    send_message(&broadcast, welcome);
	    free_message(welcome);
	
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
