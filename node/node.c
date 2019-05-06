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

//states
#define UNCONNECTED 0
#define CONNECTED 1

//messages
#define HELLO 0
#define WELCOME 1
#define CHOOSE 2

//timeout
#define RESEND_HELLO 3
#define TIMEOUT_WELCOME 5

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

/*----- Queue managment -----------------------------------------------------*/
struct queue_elem {
	discovery_t* data;
	struct queue_elem* next;
};
struct queue {
	struct queue_elem* head;
	struct queue_elem* tail;
	int size;
};
typedef struct queue queue_t;

void queue_add(queue_t* queue, discovery_t* data) {
	struct queue_elem* elem = (struct queue_elem*) malloc(sizeof(struct queue_elem));
	if (elem == NULL) {
		printf("Error assigning memory");
		exit(1);
	}
	
	elem->data = data;
	elem->next = 0;
	
	queue->tail->next = elem;
	queue->tail = elem;
	
	if (queue->head == 0){
		queue->head = elem;
	}
	queue->size++;
}

discovery_t* queue_pop(queue_t* queue) {
    if (queue->size == 0) return NULL;
    
	discovery_t* data = queue->head->data;
	struct queue_elem* old_head = queue->head;
	if (queue->head == queue->tail)
		queue->tail = 0;
	
	if (queue->head->next != 0)
		queue->head = queue->head->next;
	else
		queue->head = 0;
	
	free(old_head);
	queue->size--;
	
	return data;
}
/*---------------------------------------------------------------------------*/

// global variables
uint8_t weight = 255;
uint8_t state = UNCONNECTED;
rimeaddr_t parent;
static struct broadcast_conn broadcast;
static struct unicast_conn uc;
static struct etimer connectedt;
queue_t packet_queue;

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
	static struct etimer et;
	
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
