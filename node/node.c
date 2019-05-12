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

//states
#define OFFLINE 0
#define CONNECTED 1

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
#define DATA 9

//timeout (seconds)
/*
 * 
 * 
 */
#define HELLO_D_MIN 3
#define HELLO_D_MAX 6
#define KEEP_ALIVE_D_MIN 10
#define KEEP_ALIVE_D_MAX 15
#define ONLINE_D_MIN 30
#define ONLINE_D_MAX 45
#define ROUTE_D_MIN 7
#define ROUTE_D_MAX 9
#define WELCOME_D_MIN 3
#define WELCOME_D_MAX 5
#define ROOT_CHECK_D_MIN 3
#define ROOT_CHECK_D_MAX 5

/*---------------------------------------------------------------------------*/
PROCESS(node_tree, "Routing tree discovery");
PROCESS(node_data, "Data transfer");
AUTOSTART_PROCESSES(&node_tree, &node_data);

/*---------------------------------------------------------------------------*/

uint8_t debug = 0;

// global variables
uint8_t state = OFFLINE;
uint8_t weight = MAX_WEIGHT;
rimeaddr_t parent;
table_t routes;

static struct ctimer hellot;
static struct ctimer keep_alivet;
static struct ctimer onlinet;
static struct ctimer routet;
static struct ctimer welcomet;
static struct ctimer root_checkt;

static struct broadcast_conn discovery_broadcast;
static struct unicast_conn maintenance_unicast;

static struct runicast_conn managment_runicast;
static struct runicast_conn data_runicast;

/*----- Common methods ------------------------------------------------------*/
void choose_parent(rimeaddr_t addr, uint8_t w) {
    if (w == MAX_WEIGHT) weight = MAX_WEIGHT;
    else weight = w + 1;
    parent = addr;
	state = CONNECTED;
	
	printf("New parent chosen : %d.%d\n", parent.u8[0], parent.u8[1]);
	
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
	weight = MAX_WEIGHT;
	parent = rimeaddr_node_addr;
	
	ctimer_restart(&hellot);
	ctimer_stop(&keep_alivet);
	ctimer_stop(&onlinet);
	ctimer_stop(&routet);
	
	flush_table(&routes);
}

void parent_refresh(uint8_t w) {
    if (w == MAX_WEIGHT) weight = MAX_WEIGHT;
    else weight = w + 1;
    ctimer_restart(&keep_alivet);
    ctimer_restart(&onlinet);
}

/*---------------------------------------------------------------------------*/

static void discovery_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
	discovery_u message;
	message.c = (char*) packetbuf_dataptr();
	uint8_t msg = message.st->msg;
	uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];
	
	uint8_t parent_cmp = my_addr_cmp(parent, *from);
	if (parent_cmp == 0 && msg != HELLO) {
		parent_refresh(w);
	}

	switch(msg) {
		case HELLO:
			if (debug) printf("HELLO message received from %d.%d\n", u0, u1);
			if (state == CONNECTED) {
			    if (my_addr_cmp(parent, *from) != 0) {
			        // send welcome
			        if (ctimer_expired(&welcomet)) ctimer_restart(&welcomet);
			    } else {
			        // parent lost connection
			        weight = MAX_WEIGHT;
			    }
			}
			break;
		case WELCOME:
			if (debug) printf("WELCOME message received from %d.%d: '%d'\n", u0, u1, w);
		    if (w < weight - 1) {
		        choose_parent(*from, w);
		    }
		    break;
		case ROOT_CHECK:
		    if (debug) printf("ROOT_CHECK message received from %d.%d: '%d'\n", u0, u1, w);
		    if (w < weight - 1) {
		        choose_parent(*from, w);
		    } 
		    // broadcast root_check
		    if (state == CONNECTED && parent_cmp == 0 && ctimer_expired(&root_checkt)) ctimer_restart(&root_checkt);
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
    
    uint8_t parent_cmp = my_addr_cmp(parent, *from);
    if (parent_cmp == 0) {
    	// update parent related infos
    	parent_refresh(w);
    } else if (state == CONNECTED) {
        // child is still reachable
	    insert_route(&routes, *from, *from);
    }
    
    switch(msg) {
        case KEEP_ALIVE:
			if (debug) printf("KEEP_ALIVE message received from %d.%d: '%d'\n", u0, u1, w);
			if (state == CONNECTED && parent_cmp != 0) {
	            // send welcome
	            if (ctimer_expired(&welcomet)) ctimer_restart(&welcomet);
			}
			break;
		case ROUTE:
			if (debug) printf("ROUTE message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
			insert_route(&routes, addr, *from);
			// send route ack
			maintenance_u* ack = create_maintenance_message(ROUTE_ACK, addr, weight);
			send_maintenance_message(&maintenance_unicast, from, ack);
			free_maintenance_message(ack);
			break;
		case ROUTE_ACK:
		    if (debug) printf("ROUTE_ACK message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
			route_shared(&routes, addr);
			break;
		case ROUTE_WITHDRAW:
		    if (debug) printf("ROUTE_WITHDRAW message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);			
			route_t* route = search_route(&routes, addr);
			// if route isn't in table or if route comes from sender
			if (route == NULL || my_addr_cmp(route->nexthop, *from)) {
			    delete_route(&routes, addr);
			    // send route withdraw
			    maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, addr, weight);
			    send_maintenance_message(&maintenance_unicast, &parent, withdraw);
			    free_maintenance_message(withdraw);
			}
		    break;
		default:
			printf("UNKOWN runicast message received from %d.%d: '%d'\n", u0, u1, msg);
			break;
	}
}
static const struct unicast_callbacks maintenance_callback = {maintenance_recv};

/*---------------------------------------------------------------------------*/
static void send_hello(void* ptr) {
    if (state == OFFLINE) {
        discovery_u* message = create_discovery_message(HELLO, weight);
        send_discovery_message(&discovery_broadcast, message);
        free_discovery_message(message);
    }
    
    ctimer_restart(&hellot);
}

void send_welcome(void* ptr) {
    discovery_u* welcome = create_discovery_message(WELCOME, weight);
    send_discovery_message(&discovery_broadcast, welcome);
    free_discovery_message(welcome);
}

void send_root_check(void* ptr) {
    discovery_u* root_check = create_discovery_message(ROOT_CHECK, weight);
    send_discovery_message(&discovery_broadcast, root_check);
    free_discovery_message(root_check);
}

static void send_keep_alive(void* ptr) {
    if (state == CONNECTED) {
        maintenance_u* message = create_maintenance_message(KEEP_ALIVE, rimeaddr_node_addr, weight);
        send_maintenance_message(&maintenance_unicast, &parent, message);
        free_maintenance_message(message);
    }
    
    ctimer_restart(&keep_alivet);
}

static void send_route(void* ptr) {
    if (state == CONNECTED) {
        // send route
        route_t* next = next_route(&routes);
        if (next != NULL) {
            maintenance_u* message = create_maintenance_message(ROUTE, next->addr, weight);
            send_maintenance_message(&maintenance_unicast, &parent, message);
            free_maintenance_message(message);
        }
    }
    
    ctimer_restart(&routet);
}

unsigned int delay(unsigned int min, unsigned int max) {
    uint8_t u0 = rimeaddr_node_addr.u8[0];
    uint8_t u1 = rimeaddr_node_addr.u8[1];
    unsigned int var = ((u1 * CLOCK_SECOND) + u0 * 17 + u1);
    return min * CLOCK_SECOND + var % ((max - min) * CLOCK_SECOND);
}

PROCESS_THREAD(node_tree, ev, data)
{
	static struct etimer et;
	
	PROCESS_EXITHANDLER(
		broadcast_close(&discovery_broadcast);
		unicast_close(&maintenance_unicast);
	)

	PROCESS_BEGIN();
	
	parent = rimeaddr_node_addr;

	broadcast_open(&discovery_broadcast, 129, &discovery_callback);
	unicast_open(&maintenance_unicast, 140, &maintenance_callback);
	
	ctimer_set(&hellot, delay(HELLO_D_MIN, HELLO_D_MAX), send_hello, NULL);
	ctimer_set(&keep_alivet, delay(KEEP_ALIVE_D_MIN, KEEP_ALIVE_D_MAX), send_keep_alive, NULL);
	ctimer_set(&onlinet, delay(ONLINE_D_MIN, ONLINE_D_MAX), disconnect, NULL);
	ctimer_set(&routet, delay(ROUTE_D_MIN, ROUTE_D_MAX), send_route, NULL);
	ctimer_set(&welcomet, delay(WELCOME_D_MIN, WELCOME_D_MAX), send_welcome, NULL);
	ctimer_set(&root_checkt, delay(ROOT_CHECK_D_MIN, ROOT_CHECK_D_MAX), send_root_check, NULL);
	
	ctimer_stop(&keep_alivet);
	ctimer_stop(&onlinet);
	ctimer_stop(&routet);
	ctimer_stop(&welcomet);
	ctimer_stop(&root_checkt);
	
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
    rimeaddr_t addr = message.st->addr;
    uint8_t w = message.st->weight;
    
    switch(msg) {
		case SUBSCRIBE:
			printf("SUBSCRIBE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			if (my_addr_cmp(parent, *from) == 0) {
			    parent_refresh(w);
			    
			    if (my_addr_cmp(addr, rimeaddr_node_addr) == 0) {
			        // TODO process subscribe
			    } else {
			        // transfer to child
			        route_t* route = search_route(&routes, addr);
			        if (route != NULL) {
			            manage_u* subscribe = create_manage_message(SUBSCRIBE, addr, weight);
			            send_manage_message(&managment_runicast, &(route->nexthop), subscribe);
			            free_manage_message(subscribe);
			        } else {
			            // tell parent that no more route exist
			            maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, addr, weight);
			            send_maintenance_message(&maintenance_unicast, &parent, withdraw);
			            free_maintenance_message(withdraw);
			        }
			    }
			}
			break;
		case UNSUBSCRIBE:
			printf("UNSUBSCRIBE message received from %d.%d: '%d.%d'\n", 
				from->u8[0], from->u8[1], addr.u8[0], addr.u8[1]);
			if (my_addr_cmp(parent, *from) == 0) {
			    parent_refresh(w);
			    
			    if (my_addr_cmp(addr, rimeaddr_node_addr) == 0) {
			        // TODO process unsubscribe
			    } else {
			        // transfer to child
			        route_t* route = search_route(&routes, addr);
			        if (route != NULL) {
			            manage_u* unsubscribe = create_manage_message(UNSUBSCRIBE, addr, weight);
			            send_manage_message(&managment_runicast, &(route->nexthop), unsubscribe);
			            free_manage_message(unsubscribe);
			        } else {
			            // tell parent that no more route exist
			            maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, addr, weight);
			            send_maintenance_message(&maintenance_unicast, &parent, withdraw);
			            free_maintenance_message(withdraw);
			        }
			    }
			}
			break;
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
    
    //route is still in use
    insert_route(&routes, addr, *from);
    
    switch(msg) {
        case DATA:
            printf("DATA received from %d.%d\n", from->u8[0], from->u8[1]);
	        // transfer data to parent
	        send_data_message(&data_runicast, &parent, &data);
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
