#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "discovery-message.h"
#include "maintenance-message.h"
#include "route-message.h"
#include "managment-message.h"
#include "data-message.h"
#include "routing-table.h"

//states
#define OFFLINE 0
#define CONNECTED 1

//messages
// discovery
#define HELLO 0
#define WELCOME 1

//maintenance
#define KEEP_ALIVE 2
#define ALIVE 3

//table sharing
#define ROUTE 4
#define ROUTE_ACK 5

//managment
#define SUBSCRIBE 6
#define UNSUBSCRIBE 7

//data
#define DATA 8

//timeout (seconds)
#define HELLO_D 5
#define KEEP_ALIVE_D 10
#define ONLINE_D 30
#define ROUTE_D 10

/*---------------------------------------------------------------------------*/
PROCESS(node_tree, "Routing tree discovery");
PROCESS(node_data, "Data transfer");
AUTOSTART_PROCESSES(&node_tree, &node_data);

/*---------------------------------------------------------------------------*/

// global variables
uint8_t state = OFFLINE;
uint8_t weight = 255;
rimeaddr_t parent;
table_t routes;

unsigned long delay_hello = HELLO_D * CLOCK_SECOND;
unsigned long delay_keep_alive = KEEP_ALIVE_D * CLOCK_SECOND;
unsigned long delay_online = ONLINE_D * CLOCK_SECOND;
unsigned long delay_route = ROUTE_D * CLOCK_SECOND;

static struct ctimer hellot;
static struct ctimer keep_alivet;
static struct ctimer onlinet;
static struct ctimer routet;

static struct broadcast_conn discovery_broadcast;
static struct unicast_conn maintenance_unicast;
static struct unicast_conn route_unicast;

static struct runicast_conn managment_runicast;
static struct runicast_conn data_runicast;

/*----- Common methods ------------------------------------------------------*/
void choose_parent(rimeaddr_t addr) {
	parent = addr;
	printf("New parent chosen : %d.%d\n", parent.u8[0], parent.u8[1]);
	
	state = CONNECTED;
	ctimer_stop(&hellot);
	ctimer_restart(&keep_alivet);
	ctimer_restart(&onlinet);
	ctimer_restart(&routet);
	
	reset_routes(&routes);
	insert_route(&routes, rimeaddr_node_addr, rimeaddr_node_addr);
}

void disconnect() {
    printf("Disconnecting...\n");
	state = OFFLINE;
	weight = 255;
	parent = rimeaddr_node_addr;
	
	ctimer_restart(&hellot);
	ctimer_stop(&keep_alivet);
	ctimer_stop(&onlinet);
	ctimer_stop(&routet);
	
	flush_table(&routes);
}

/*---------------------------------------------------------------------------*/

static void discovery_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
	discovery_u message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t msg = message.st->msg;
	uint8_t w = message.st->msg;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];

	switch(msg) {
		case HELLO:
			//printf("HELLO message received from %d.%d\n", u0, u1);
			if (state == CONNECTED) {
			    if (addr_cmp(parent, *from) != 0) {
			        // send welcome
			        discovery_u* message = create_discovery_message(WELCOME, weight);
			        send_discovery_message(&discovery_broadcast, message);
			        free_discovery_message(message);
			    } else {
			        weight = 255;
			    }
			}
			break;
		case WELCOME:
			//printf("WELCOME message received from %d.%d: '%d'\n", u0, u1, w);
		    if (w < weight - 1) {
		        weight = w + 1;
		        choose_parent(*from);
		    } else if (state == CONNECTED && addr_cmp(parent, *from) == 0) {
		        ctimer_restart(&keep_alivet);
			    ctimer_restart(&onlinet);
			    // TODO broadcast welcome
		    }
		    break;
		default:
			printf("UNKOWN broadcast message received from %d.%d: '%d'\n", u0, u1, 
				msg);
			break;
	}
}
static const struct broadcast_callbacks discovery_callback = {discovery_recv};

/*---------------------------------------------------------------------------*/

static void maintenance_recv(struct unicast_conn *c, const rimeaddr_t *from) {
	maintenance_u message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t msg = message.st->msg;
	uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];

	switch(msg) {
		case KEEP_ALIVE:
			//printf("KEEP_ALIVE message received from %d.%d: '%d'\n", u0, u1, w);
			if (state == CONNECTED && addr_cmp(parent, *from) != 0) {
			    // send alive
			    maintenance_u* message = create_maintenance_message(ALIVE, weight);
			    send_maintenance_message(&maintenance_unicast, from, message);
			    free_maintenance_message(message);
			}
			break;
		case ALIVE:
			//printf("ALIVE message received from %d.%d: '%d'\n", u0, u1, w);
		    if (state == CONNECTED && addr_cmp(parent, *from) == 0) {
		        if (w == 255) weight = 255;
		        else weight = w + 1;
		        ctimer_restart(&keep_alivet);
		        ctimer_restart(&onlinet);
			}
			break;
		default:
			printf("UNKOWN unicast message received from %d.%d: '%d'\n", u0, u1, 
				msg);
			break;
	}
}
static const struct unicast_callbacks maintenance_callback = {maintenance_recv};
/*---------------------------------------------------------------------------*/

static void route_recv(struct unicast_conn *c, const rimeaddr_t *from)
{
    route_u message;
    message.c = (char *) packetbuf_dataptr();
    rimeaddr_t addr = message.st->addr;
    
    switch(message.st->msg) {
		case ROUTE:
			printf("ROUTE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			insert_route(&routes, addr, *from);
			// send route ack
			route_u* message = create_route_message(ROUTE_ACK, addr);
			send_route_message(&route_unicast, *from, message);
			free_route_message(message);
			break;
		case ROUTE_ACK:
		    printf("ROUTE_ACK message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			if (state == CONNECTED && addr_cmp(parent, *from) == 0) {
			    ctimer_restart(&keep_alivet);
			    ctimer_restart(&onlinet);
			}
			route_t* route = search_route(&routes, addr);
			if (route != NULL) route->shared = 1;
		default:
			printf("UNKOWN runicast message received from %d.%d: '%d'\n", from->u8[0], 
				from->u8[1], message.st->msg);
			break;
	}
}
static const struct unicast_callbacks route_callback = {route_recv};

/*---------------------------------------------------------------------------*/
static void send_hello(void* ptr) {
    ctimer_restart(&hellot);
    
    if (state == OFFLINE) {
        discovery_u* message = create_discovery_message(HELLO, weight);
        send_discovery_message(&discovery_broadcast, message);
        free_discovery_message(message);
    }
}

static void send_keep_alive(void* ptr) {
    ctimer_restart(&keep_alivet);
    
    if (state == CONNECTED) {
        maintenance_u* message = create_maintenance_message(KEEP_ALIVE, weight);
        send_maintenance_message(&maintenance_unicast, &parent, message);
        free_maintenance_message(message);
    }
}

static void send_route(void* ptr) {
    ctimer_restart(&routet);
    
    if (state == CONNECTED) {
        // send route
        route_t* next = next_route(&routes);
        if (next == NULL) return;
        
        route_u* message = create_route_message(ROUTE, next->addr);
        send_route_message(&route_unicast, &parent, message);
        free_route_message(message);
    }
}

PROCESS_THREAD(node_tree, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		broadcast_close(&discovery_broadcast);
		unicast_close(&maintenance_unicast);
		unicast_close(&route_unicast);
	)

	PROCESS_BEGIN();

	broadcast_open(&discovery_broadcast, 129, &discovery_callback);
	unicast_open(&maintenance_unicast, 140, &maintenance_callback);
	unicast_open(&route_unicast, 151, &route_callback);
	
	ctimer_set(&hellot, delay_hello + rimeaddr_node_addr.u8[0] * 10, send_hello, NULL);
	ctimer_set(&keep_alivet, delay_keep_alive + rimeaddr_node_addr.u8[0] * 10, send_keep_alive, NULL);
	ctimer_set(&onlinet, delay_online + rimeaddr_node_addr.u8[0] * 10, disconnect, NULL);
	ctimer_set(&routet, delay_route + rimeaddr_node_addr.u8[0] * 10, send_route, NULL);
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
    rimeaddr_t addr = message.st->addr;
    
    switch(message.st->msg) {
		case SUBSCRIBE:
			printf("SUBSCRIBE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			if (addr_cmp(parent, *from) == 0) {
			    ctimer_restart(&keep_alivet);
			    ctimer_restart(&onlinet);
			    
			    if (addr_cmp(addr, rimeaddr_node_addr) == 0) {
			        // TODO process subscribe
			    } else {
			        // TODO transfer to child
			    }
			}
			break;
		case UNSUBSCRIBE:
			printf("UNSUBSCRIBE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			if (addr_cmp(parent, *from) == 0) {
			    ctimer_restart(&keep_alivet);
			    ctimer_restart(&onlinet);
			    
			    if (addr_cmp(addr, rimeaddr_node_addr) == 0) {
			        // TODO process unsubscribe
			    } else {
			        // TODO transfer to child
			    }
			}
			break;
		default:
			printf("UNKOWN runicast message received from %d.%d: '%d'\n", from->u8[0], 
				from->u8[1], message.st->msg);
			break;
	}
}

static void recv_data(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
    char* message = (char *) packetbuf_dataptr();
    printf("DATA received from %d.%d: '%s'\n", 
				from->u8[0], from->u8[1], message);
	// TODO transfer data to parent
}

static void sent(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {}

static void timedout(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
		to->u8[0], to->u8[1], retransmissions);

	disconnect();
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
