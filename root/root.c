#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "routing-table.h"
#include "routing-message.h"
#include "broadcast-managment.h"

//messages
#define HELLO 0
#define WELCOME 1

//routing states
#define READY 0
#define OCCUPIED 1

//routing
#define ROUTE 3
#define ROUTE_ACK 4

//data
#define ROOT 5
#define NODE 6

//timeout
#define RESEND_WELCOME 3

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/

// global variables
static struct broadcast_conn broadcast;
static struct runicast_conn routing_runicast;
static struct unicast_conn payload_unicast;
table_t table;

/*---------------------------------------------------------------------------*/
static void recv_routing(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
    routing_u message;
    message.c = (char *) packetbuf_dataptr();
    rimeaddr_t addr = message.st->addr;
    
    switch(message.st->msg) {
		case ROUTE:
			printf("ROUTE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			insert_route(&table, addr, *from);
			break;
		default:
			printf("UNKOWN message received from %d.%d: '%d'\n", from->u8[0], 
				from->u8[1], message.st->msg);
			break;
	}
}
static void sent_routing(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {}

static void timedout_routing(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {}

static const struct runicast_callbacks routing_callbacks = {
	recv_routing,
    sent_routing,
    timedout_routing
};

/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from) {}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_broadcast_process, ev, data)
{
	
	PROCESS_EXITHANDLER(
	    broadcast_close(&broadcast);
	    runicast_close(&routing_runicast);
	)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&routing_runicast, 146, &routing_callbacks);
	
	static struct etimer welcomet;
	
    etimer_set(&welcomet, CLOCK_SECOND * RESEND_WELCOME);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&welcomet));
        
        etimer_reset(&welcomet);
        
	    discovery_t* welcome = create_broadcast_message(WELCOME, 0);
	    send_broadcast_message(&broadcast, welcome);
	    free_broadcast_message(welcome);
	
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
