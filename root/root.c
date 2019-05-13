#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "discovery-message.h"
#include "maintenance-message.h"
#include "managment-message.h"
#include "data-message.h"
#include "routing-table.h"

// maximum weight
#define MAX_WEIGHT 255

//messages
// discovery
#define HELLO 0
#define WELCOME 1
#define ROOT_CHECK 2

//maintenance
#define KEEP_ALIVE 3
#define ROUTE 4
#define ROUTE_ACK 5
#define ROUTE_WITHDRAW 6

//managment
#define SUBSCRIBE 7
#define UNSUBSCRIBE 8

//data
#define TEMPERATURE 9
#define HUMIDITY 10

//timeout (seconds)
/*
 * 
 * 
 */
#define WELCOME_D 1
#define ROOT_CHECK_D 60

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

static struct runicast_conn managment_runicast;
static struct runicast_conn data_runicast;

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
			if (debug_rcv) printf("ROUTE message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
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
			// if route isn't in table or if route comes from sender
			if (route == NULL || my_addr_cmp(route->nexthop, *from)) {
			    delete_route(&routes, addr);
			}
		    break;
		default:
			printf("UNKOWN runicast message received from %d.%d: '%d'\n", u0, u1, msg);
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
static void recv_managment(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
    manage_u message;
    message.c = (char *) packetbuf_dataptr();
    uint8_t msg = message.st->msg;
    //rimeaddr_t addr = message.st->addr;
    //uint8_t w = message.st->weight;
    
    switch(msg) {
		default:
			printf("UNKOWN runicast message received from %d.%d: '%d'\n", from->u8[0], 
				from->u8[1], msg);
			break;
	}
}

static void recv_data(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
    data_u data;
    data.c = (char*) packetbuf_dataptr();
    uint8_t msg = data.st->msg;
    rimeaddr_t addr = data.st->addr;
    uint8_t mesure = data.st->value;
    
    //route is still in use
    insert_route(&routes, addr, *from);
    
    switch(msg) {
        case TEMPERATURE:
            printf("TEMPERATURE received from %d.%d : '%d.%d -> %d\n", from->u8[0], from->u8[1], addr.u8[0], addr.u8[1], mesure);
            
            // TODO process temperature
            break;
        case HUMIDITY:
            printf("HUMIDITY received from %d.%d : '%d.%d -> %d\n", from->u8[0], from->u8[1], addr.u8[0], addr.u8[1], mesure);
            
            // TODO process humidity
            break;
        default:
            printf("UNKOWN runicast message received from %d.%d: '%d'\n", from->u8[0], 
				from->u8[1], msg);
            break;
    }
}

static void sent(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {}

static void timedout(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
		to->u8[0], to->u8[1], retransmissions);

}

static const struct runicast_callbacks managment_callbacks = {
	recv_managment,
    sent,
    timedout
};

static const struct runicast_callbacks data_callbacks = {
    recv_data,
    sent,
    timedout
};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(node_data, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		runicast_close(&managment_runicast);
		runicast_close(&data_runicast);
	)

	PROCESS_BEGIN();

	runicast_open(&managment_runicast, 162, &managment_callbacks);
	runicast_open(&data_runicast, 173, &data_callbacks);
	
	etimer_set(&et, CLOCK_SECOND);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		etimer_restart(&et);
	}

	PROCESS_END();
}
