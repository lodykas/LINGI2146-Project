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
#include <string.h>

//messages
#define HELLO 0
#define WELCOME 1

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Routing tree discovery");
AUTOSTART_PROCESSES(&example_broadcast_process);

struct discovery_struct {
	uint8_t msg;
	uint8_t weigth;
} ;
typedef struct discovery_struct discovery_struct_t;

union discovery_union {
	char* c;
	discovery_struct_t* st;
};
typedef union discovery_union discovery_t;



struct queue_elem {
	discovery_t* data;
	struct queue_elem* next;
};
struct queue{
	struct queue_elem* head;
	struct queue_elem* tail;
};
typedef struct queue queue_t;

void queue_add(queue_t* queue, discovery_t* data){
	struct queue_elem* elem = (struct queue_elem*) malloc(sizeof(struct queue_elem));
	if(elem == 0){
		printf("error assigning memory");
		exit(1);
	}
	
	elem->data = data;
	elem->next = 0;
	
	queue->tail->next = elem;
	queue->tail = elem;
	
	if(queue->head == 0){
		queue->head = elem;
	}
}
discovery_t* queue_pop(queue_t* queue){

	discovery_t* data = queue->head->data;
	struct queue_elem* old_head = queue->head;
	if(queue->head == queue->tail){
		queue->tail = 0;
	}
	
	if(queue->head->next != 0){
		queue->head = queue->head->next;
	}
	else{
		queue->head = 0;
	}
	free(old_head);
	return data;
}



uint8_t weight = 255; 
static struct broadcast_conn broadcast;
queue_t packet_queue;


/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	discovery_t message;
	message.c = (char*) packetbuf_dataptr();
	printf("MSG: %x\n",message.st->msg);
		 
	discovery_t response;
	switch(message.st->msg){
		case HELLO:
			printf("HELLO message received from %d.%d: '%d'\n",from->u8[0], from->u8[1], message.st->msg);
			response.st->msg = WELCOME;
			response.st->weigth = weight;
			break;
		case WELCOME:
			printf("WELCOME message received from %d.%d: '%d'\n",from->u8[0], from->u8[1], message.st->msg);
			break;
		default:
			break;
		
	}
	queue_add(&packet_queue,&response);
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
	static struct etimer et;

	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	
	/* Send hello */
	discovery_t msg;
	msg.st->msg = HELLO;
	msg.st->weigth = 0;
	

	while(1) {
	
		printf("SENT: %x\n",msg.st->msg);
		packetbuf_copyfrom(msg.c, sizeof(discovery_struct_t));
		broadcast_send(&broadcast);

		/* Delay 2-4 seconds */
		etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		while(packet_queue.head != 0){
			discovery_t* to_send;
			to_send = queue_pop(&packet_queue);
			packetbuf_copyfrom(to_send->c, sizeof(discovery_struct_t));
			broadcast_send(&broadcast);
			free(to_send);
		}
	
	
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
