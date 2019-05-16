#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "discovery-message.h"
#include "maintenance-message.h"
#include "sensor-message.h"
#include "routing-table.h"

// maximum weight
#define MAX_WEIGHT 255

//messages
// discovery
#define HELLO 0
#define WELCOME 1
#define ROOT_CHECK 2

//maintenance
#define KEEP_ALIVE 0
#define ROUTE 1
#define ROUTE_ACK 2
#define ROUTE_WITHDRAW 3

//down data
#define PERIODIC_SEND 0
#define ON_CHANGE_SEND 1
#define DONT_SEND 2

//up data
#define TEMPERATURE 0
#define HUMIDITY 1

//ack
#define ACK 1

// !! can not exceed 16 (max seqnum is 15)
#define SENDING_QUEUE_SIZE 16

//timeout (seconds)
/*
 * 
 * 
 */
#define WELCOME_D 1
#define ROOT_CHECK_D 60
#define DATA_D 5

/*---------------------------------------------------------------------------*/
PROCESS(node_tree, "Routing tree discovery");
PROCESS(node_data, "Data transfer");
AUTOSTART_PROCESSES(&node_tree, &node_data);

/*---------------------------------------------------------------------------*/

uint8_t debug_rcv = 0;
uint8_t debug_snd = 0;

// global variables
table_t routes;

static struct ctimer welcomet;
static struct ctimer root_checkt;

static struct broadcast_conn discovery_broadcast;
static struct unicast_conn maintenance_unicast;

/*---------------------------------------------------------------------------*/

static void discovery_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
	discovery_u message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t msg = message.st->msg;
	//uint8_t w = message.st->msg;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];

	switch(msg) {
		case HELLO:
			if (debug_rcv) printf("HELLO message received from %d.%d\n", u0, u1);
		    // send welcome
		    if (ctimer_expired(&welcomet)) ctimer_restart(&welcomet);
			break;
		case WELCOME:
			if (debug_rcv) printf("WELCOME message received from %d.%d\n", u0, u1);
			// do nothing
			break;
		case ROOT_CHECK:
			if (debug_rcv) printf("ROOT_CHECK message received from %d.%d\n", u0, u1);
			// do nothing
			break;
		default:
			printf("UNKOWN broadcast message received from %d.%d: '%d'\n", u0, u1, 
				msg);
			break;
	}
}
static const struct broadcast_callbacks discovery_callback = {discovery_recv};

/*---------------------------------------------------------------------------*/

static void maintenance_recv(struct unicast_conn *c, const rimeaddr_t *from)
{
    maintenance_u message;
    message.c = (char *) packetbuf_dataptr();
    uint8_t msg = message.st->msg;
    rimeaddr_t addr = message.st->addr;
    uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];
    
	insert_route(&routes, *from, *from);
    
    switch(msg) {
        case KEEP_ALIVE:
			if (debug_rcv) printf("KEEP_ALIVE message received from %d.%d: '%d'\n", u0, u1, w);
            // send welcome
            if (ctimer_expired(&welcomet)) ctimer_restart(&welcomet);
			break;
		case ROUTE:
			/*if (debug_rcv)*/ printf("ROUTE message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
			insert_route(&routes, addr, *from);
			// send route ack
			maintenance_u* ack = create_maintenance_message(ROUTE_ACK, addr, 0);
			send_maintenance_message(&maintenance_unicast, from, ack);
			free_maintenance_message(ack);
			break;
		case ROUTE_ACK:
			if (debug_rcv) printf("ROUTE_ACK message received from %d.%d: '%d'\n", u0, u1, w);
            // do nothing
			break;
		case ROUTE_WITHDRAW:
		    if (debug_rcv) printf("ROUTE_WITHDRAW message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);			
			route_t* route = search_route(&routes, addr);
			// if route is in table or if route comes from sender
			if (route != NULL && my_addr_cmp(route->nexthop, *from) == 0) {
			    delete_route(&routes, addr);
			}
		    break;
		default:
			printf("UNKOWN maintenance message received from %d.%d: '%d'\n", u0, u1, msg);
			break;
	}
}
static const struct unicast_callbacks maintenance_callback = {maintenance_recv};

/*---------------------------------------------------------------------------*/

void send_welcome(void* ptr) {
    if (debug_snd) printf("Sending welcome...\n");
    discovery_u* welcome = create_discovery_message(WELCOME, 0);
    send_discovery_message(&discovery_broadcast, welcome);
    free_discovery_message(welcome);
}

void send_root_check(void* ptr) {
    if (debug_snd) printf("Sending root check...\n");
    discovery_u* root_check = create_discovery_message(ROOT_CHECK, 0);
    send_discovery_message(&discovery_broadcast, root_check);
    free_discovery_message(root_check);
    
    ctimer_restart(&root_checkt);
}

PROCESS_THREAD(node_tree, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		broadcast_close(&discovery_broadcast);
		unicast_close(&maintenance_unicast);
	)

	PROCESS_BEGIN();

	broadcast_open(&discovery_broadcast, 129, &discovery_callback);
	unicast_open(&maintenance_unicast, 140, &maintenance_callback);
	
	ctimer_set(&welcomet, WELCOME_D * CLOCK_SECOND, send_welcome, NULL);
	ctimer_set(&root_checkt, ROOT_CHECK_D * CLOCK_SECOND, send_root_check, NULL);
	
	etimer_set(&et, CLOCK_SECOND);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); 
        
		etimer_restart(&et);
	}
	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* DATA sharing                                                              */
/*---------------------------------------------------------------------------*/

sensor_u* downs[SENDING_QUEUE_SIZE];
uint8_t sending_cursor_down = 0;
uint8_t receiving_cursor_down = 0;

static struct ctimer downt;

static struct unicast_conn sensor_up_unicast;
static struct unicast_conn sensor_down_unicast;

/*---------------------------------------------------------------------------*/

void send_down(void* ptr) {
    sensor_u* d = downs[sending_cursor_down];
    uint8_t cnt = 0;
    while (d == NULL && cnt < SENDING_QUEUE_SIZE) { 
        sending_cursor_down = (sending_cursor_down + 1) % SENDING_QUEUE_SIZE;
        d = downs[sending_cursor_down];
        cnt++;
    }
    if (d != NULL) {
        route_t* r = search_route(&routes, d->st->addr);
        if (r != NULL) {
            send_sensor_message(&sensor_down_unicast, &(r->nexthop), d);
        }
        sending_cursor_down = (sending_cursor_down + 1) % SENDING_QUEUE_SIZE;
    }
    
    ctimer_restart(&downt);
}

void add_down(uint8_t message, rimeaddr_t addr) {
    sensor_u* d = downs[receiving_cursor_down];
    if (d != NULL) free_sensor_message(d);
    downs[receiving_cursor_down] = create_sensor_message(message, 0, receiving_cursor_down, addr, 0);
    receiving_cursor_down = (receiving_cursor_down + 1) % SENDING_QUEUE_SIZE;
}

void ack_down(uint8_t seqnum, uint8_t msg, rimeaddr_t addr) {
    sensor_u* d = downs[seqnum];
    if (d != NULL && get_msg(d) == msg && my_addr_cmp(d->st->addr, addr) == 0) free_sensor_message(d);
    downs[seqnum] = NULL;
}

/*---------------------------------------------------------------------------*/
static void recv_down(struct unicast_conn *c, const rimeaddr_t *from) {}

static void recv_up(struct unicast_conn *c, const rimeaddr_t *from)
{
    sensor_u message;
    message.c = (char*) packetbuf_dataptr();
    
    uint8_t ack = get_ack(&message);
    uint8_t seqnum = get_seqnum(&message);
    uint8_t msg = get_msg(&message);
    rimeaddr_t addr = message.st->addr;
    
    // if it is an ack, we remove the packet from our sending queue
    if (ack) {
        ack_down(seqnum, msg, addr);
        return;
    }
    
    // TODO process mesure received
    printf("Received mesure from %d.%d : '%d' = %d\n", addr.u8[0], addr.u8[1], msg, message.st->value);
    
    // send ack
    sensor_u* a = create_sensor_message(msg, ACK, seqnum, addr, 0);
    send_sensor_message(&sensor_down_unicast, from, a);
    free_sensor_message(a);
}

static const struct unicast_callbacks sensor_down_callbacks = {recv_down};

static const struct unicast_callbacks sensor_up_callbacks = {recv_up};


/*---------------------------------------------------------------------------*/

PROCESS_THREAD(node_data, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		unicast_close(&sensor_up_unicast);
		unicast_close(&sensor_down_unicast);
	)

	PROCESS_BEGIN();

	unicast_open(&sensor_up_unicast, 162, &sensor_up_callbacks);
	unicast_open(&sensor_down_unicast, 173, &sensor_down_callbacks);
	
	ctimer_set(&downt, DATA_D * CLOCK_SECOND, send_down, NULL);
	etimer_set(&et, CLOCK_SECOND);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		etimer_restart(&et);
	}

	PROCESS_END();
}
