#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "routing-table.h"
#include "broadcast-managment.h"

//states
#define UNCONNECTED 0
#define CONNECTED 1
#define SHARING 2

//messages
#define HELLO 0
#define WELCOME 1
#define CHOOSE 2
#define ROUTE 3

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
rimeaddr_t parent;
table_t table;

static struct broadcast_conn broadcast;
static struct unicast_conn uc;

static struct etimer connectedt;
static struct etimer welcomet;
static struct etimer hellot;

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
void choose_parent(unsigned char u0, unsigned char u1) {
	parent.u8[0] = u0;
	parent.u8[1] = u1;
	printf("New parent chosen : %d.%d\n", u0, u1);
	
	discovery_t* choose = create_message(CHOOSE, weight);
	packetbuf_copyfrom(choose->c, sizeof(discovery_struct_t));
	if(!rimeaddr_cmp(&parent, &rimeaddr_node_addr)) {
		unicast_send(&uc, &parent);
	}
	free_message(choose);
}

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
	discovery_t message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];

	switch(message.st->msg) {
		case HELLO:
			printf("HELLO message received from %d.%d: '%d'\n", u0, u1, w);
	
			if (state == CONNECTED) {
			    clock_time_t d = delay_welcome + random_rand() % delay_welcome;
				etimer_set(&welcomet, d);
			    state = SHARING;
			}
			break;
		case WELCOME:
			printf("WELCOME message received from %d.%d: '%d'\n", u0, u1, w);
			
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
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	unicast_open(&uc, 146, &unicast_callbacks);
	
	etimer_set(&hellot, 0);
	etimer_set(&et, CLOCK_SECOND / 10);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(
        	etimer_expired(&et)
        );
		
		etimer_reset(&et);
	
		switch (state) {
			case UNCONNECTED:
				if (etimer_expired(&hellot) != 0) {
					discovery_t* hello = create_message(HELLO, 0);
					send_message(&broadcast, hello);
					free_message(hello);
					etimer_set(&hellot, delay_hello + random_rand() % delay_hello);
				}
				break;
			case CONNECTED:
				if (etimer_expired(&connectedt) != 0) {
					printf("Connection lost...\n");
					state = UNCONNECTED;
					weight = 255;
					break;
				}
				break;
			case SHARING:
			    if (etimer_expired(&welcomet) != 0) {
			        state = CONNECTED;
				    discovery_t* welcome = create_message(WELCOME, weight);
				    send_message(&broadcast, welcome);
				    free_message(welcome);
				}
			    break;
			default:
				printf("WRONG STATE, EXITING...");
				//PROCESS_END();
				break;
		}
	
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
