#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "routing-table.h"
#include "broadcast-managment.h"

//messages
#define WELCOME 1
#define CHOOSE 2

//timeout
#define RESEND_WELCOME 3

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/

// global variables
static struct broadcast_conn broadcast;
static struct unicast_conn uc;
table_t table;
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
	
    etimer_set(&welcomet, CLOCK_SECOND * RESEND_WELCOME);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&welcomet));
        
        etimer_reset(&welcomet);
        
	    discovery_t* welcome = create_message(WELCOME, 0);
	    send_message(&broadcast, welcome);
	    free_message(welcome);
	
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
