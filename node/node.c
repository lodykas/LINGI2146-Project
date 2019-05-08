#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "routing-table.h"
#include "routing-message.h"
#include "broadcast-managment.h"

//states
#define UNCONNECTED 0
#define CONNECTED 1
#define SHARING 2

//routing states
#define READY 0
#define OCCUPIED 1

//messages
#define HELLO 0
#define WELCOME 1

//routing
#define ROUTE 3
#define ROUTE_ACK 4

//data
#define ROOT 5
#define NODE 6

//timeout
#define RESEND_HELLO 3
#define SEND_WELCOME 1
#define TIMEOUT_WELCOME 10

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);

/*---------------------------------------------------------------------------*/

// global variables
int delay_hello = RESEND_HELLO * CLOCK_SECOND;
int delay_welcome = SEND_WELCOME * CLOCK_SECOND;
int timeout_welcome = TIMEOUT_WELCOME * CLOCK_SECOND;

uint8_t weight = 255;
uint8_t state = UNCONNECTED;
uint8_t routing_state = READY;
double retransmission = 1.0;
rimeaddr_t parent;
table_t table;

static struct broadcast_conn broadcast;
static struct runicast_conn routing_runicast;

static struct etimer connectedt;
static struct etimer welcomet;
static struct etimer hellot;

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
static void sent_routing(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {
	routing_state = READY;
}

static void timedout_routing(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
		to->u8[0], to->u8[1], retransmissions);

	disconnect();
}

static const struct runicast_callbacks routing_callbacks = {
	recv_routing,
    sent_routing,
    timedout_routing
};

/*---------------------------------------------------------------------------*/
void choose_parent(unsigned char u0, unsigned char u1) {
	parent.u8[0] = u0;
	parent.u8[1] = u1;
	printf("New parent chosen : %d.%d\n", u0, u1);
	
	routing_state = READY;
	
	reset_routes(&table);
	
	insert_route(&table, rimeaddr_node_addr, rimeaddr_node_addr);
}

void disconnect() {
	reset_routes(&table);
	state = UNCONNECTED;
	weight = 255;
}

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
	discovery_t message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];

	switch(message.st->msg) {
		case HELLO:
			//printf("HELLO message received from %d.%d: '%d'\n", u0, u1, w);
	
			if (state == CONNECTED) {
				if (parent.u8[0] == u0 && parent.u8[1] == u1) {
					disconnect();
				} else {
				    clock_time_t d = delay_welcome + random_rand() % delay_welcome;
					etimer_set(&welcomet, d);
				    state = SHARING;
				}
			}
			break;
		case WELCOME:
			//printf("WELCOME message received from %d.%d: '%d'\n", u0, u1, w);
			
			if (state > UNCONNECTED && parent.u8[0] == u0 && parent.u8[1] == u1) {
				weight = w + 1;
				etimer_set(&connectedt, timeout_welcome);
				if (state == CONNECTED) {
					clock_time_t d = delay_welcome + random_rand() % delay_welcome;
				    etimer_set(&welcomet, d);
				    state = SHARING;
				}
			} else if (w + 1 < weight) {
				weight = w + 1;
				choose_parent(u0, u1);
				etimer_set(&connectedt, timeout_welcome);
				clock_time_t d = delay_welcome + random_rand() % delay_welcome;
				etimer_set(&welcomet, d);
			    state = SHARING;
			}
			break;
		default:
			printf("UNKOWN message received from %d.%d: '%d'\n", u0, u1, 
				message.st->msg);
			
			break;
	}
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_broadcast_process, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		broadcast_close(&broadcast);
		runicast_close(&routing_runicast);
	)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	runicast_open(&routing_runicast, 146, &routing_callbacks);
	
	etimer_set(&hellot, 0);
	etimer_set(&et, CLOCK_SECOND / 10);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(
        	etimer_expired(&et)
        );
		
		etimer_reset(&et);
	
		// Managing broadcast discovery
		switch (state) {
			case UNCONNECTED:
				if (etimer_expired(&hellot) != 0) {
					discovery_t* hello = create_broadcast_message(HELLO, 0);
					send_broadcast_message(&broadcast, hello);
					free_broadcast_message(hello);
					etimer_set(&hellot, delay_hello + random_rand() % delay_hello);
				}
				break;
			case CONNECTED:
				if (etimer_expired(&connectedt) != 0) {
					printf("Connection lost...\n");
					disconnect();
					break;
				}
				break;
			case SHARING:
			    if (etimer_expired(&welcomet) != 0) {
			        state = CONNECTED;
				    discovery_t* welcome = create_broadcast_message(WELCOME, weight);
				    send_broadcast_message(&broadcast, welcome);
				    free_broadcast_message(welcome);
				}
			    break;
			default:
				printf("WRONG STATE, EXITING... (should)");
				break;
		}
		
		// Managing route sharing
		if (state > UNCONNECTED && routing_state == READY) {
			route_t* next = next_route(&table);
			if (next != NULL) {
				printf("Sending route %d.%d\n", next->addr.u8[0], next->addr.u8[1]);
				routing_u* route = create_routing_message(ROUTE, next->addr);
				send_routing_message(&routing_runicast, &parent, route);
				free_routing_message(route);
				next->shared = 1;
				routing_state = OCCUPIED;
			}
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
