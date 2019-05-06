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

//messages
#define HELLO 0
#define WELCOME 1
#define CHOOSE 2
#define ROUTE 3

//timeout
#define RESEND_HELLO 3
#define TIMEOUT_WELCOME 5

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);


/*---------------------------------------------------------------------------*/

// global variables
uint8_t weight = 255;
uint8_t state = UNCONNECTED;
rimeaddr_t parent;
static struct broadcast_conn broadcast;
static struct unicast_conn uc;
static struct etimer connectedt;
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
				discovery_t* welcome = create_message(WELCOME, weight);
				send_message(&broadcast, welcome);
				free_message(welcome);
			}
			break;
		case WELCOME:
			printf("WELCOME message received from %d.%d: '%d'\n", u0, u1, w);
			
			if (state == CONNECTED && parent.u8[0] == u0 && parent.u8[1] == u1) {
				etimer_set(&connectedt, CLOCK_SECOND * TIMEOUT_WELCOME + 
					random_rand() % (CLOCK_SECOND * TIMEOUT_WELCOME));
				discovery_t* welcome = create_message(WELCOME, weight);
				send_message(&broadcast, welcome);
				free_message(welcome);
			} else if (w + 1 < weight) {
				weight = w + 1;
				state = CONNECTED;
				etimer_set(&connectedt, CLOCK_SECOND * TIMEOUT_WELCOME + 
					random_rand() % (CLOCK_SECOND * TIMEOUT_WELCOME));
				choose_parent(u0, u1);
				discovery_t* welcome = create_message(WELCOME, weight);
				send_message(&broadcast, welcome);
				free_message(welcome);
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
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	unicast_open(&uc, 146, &unicast_callbacks);
	
	static struct etimer hellot;
	etimer_set(&hellot, 0);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(
        	(etimer_expired(&hellot) && state == UNCONNECTED) || 
        	(etimer_expired(&connectedt) && state == CONNECTED)
        );
	
		switch (state) {
			case UNCONNECTED:
				if (etimer_expired(&hellot)) { //useless
					discovery_t* hello = create_message(HELLO, 0);
					send_message(&broadcast, hello);
					free_message(hello);
					etimer_set(&hellot, CLOCK_SECOND * RESEND_HELLO + 
						random_rand() % (CLOCK_SECOND * RESEND_HELLO));
				}
				break;
			case CONNECTED:
				if (etimer_expired(&connectedt)) { //useless
					state = UNCONNECTED;
					weight = 255;
					break;
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
