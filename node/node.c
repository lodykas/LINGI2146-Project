#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "discovery-message.h"
#include "maintenance-message.h"
#include "sensor-message.h"

#include "routing-table.h"
#include "sensor-data.h"

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
#define HELLO_D_MIN 3
#define HELLO_D_MAX 6
#define KEEP_ALIVE_D_MIN 10
#define KEEP_ALIVE_D_MAX 15
#define ONLINE_D_MIN 30
#define ONLINE_D_MAX 45
#define ROUTE_D_MIN 7
#define ROUTE_D_MAX 9
#define WELCOME_D_MIN 2
#define WELCOME_D_MAX 3
#define ROOT_CHECK_D_MIN 3
#define ROOT_CHECK_D_MAX 5
#define DATA_D_MIN 5
#define DATA_D_MAX 10

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

	switch(msg) {
		case HELLO:
			if (debug) printf("HELLO message received from %d.%d\n", u0, u1);
			if (state == CONNECTED) {
			    if (parent_cmp != 0) {
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
		    } else if (parent_cmp == 0) {
		        parent_refresh(w);
		    }
		    break;
		case ROOT_CHECK:
		    if (debug) printf("ROOT_CHECK message received from %d.%d: '%d'\n", u0, u1, w);
		    if (w < weight - 1) {
		        choose_parent(*from, w);
		    } else if (parent_cmp == 0) {
		        parent_refresh(w);
		    }
		    // broadcast root_check
		    if (state == CONNECTED && parent_cmp == 0 && ctimer_expired(&root_checkt)) ctimer_restart(&root_checkt);
		    break;
		default:
			printf("UNKOWN discovery message received from %d.%d: '%d'\n", u0, u1, 
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
			// if route in table or if route comes from sender
			if (route != NULL && my_addr_cmp(route->nexthop, *from) == 0) {
			    delete_route(&routes, addr);
			    // send route withdraw
			    maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, addr, weight);
			    send_maintenance_message(&maintenance_unicast, &parent, withdraw);
			    free_maintenance_message(withdraw);
			}
		    break;
		default:
			printf("UNKOWN maintenance message received from %d.%d: '%d'\n", u0, u1, msg);
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
/* DATA sharing                                                              */
/*---------------------------------------------------------------------------*/

uint8_t sensor_state = ON_CHANGE_SEND;

sensor_data_t* temperature;
sensor_data_t* humidity;

uint8_t last_temp = 0;
uint8_t last_hum = 0;

sensor_u* ups[SENDING_QUEUE_SIZE];
uint8_t sending_cursor_up = 0;
uint8_t receiving_cursor_up = 0;

sensor_u* downs[SENDING_QUEUE_SIZE];
uint8_t sending_cursor_down = 0;
uint8_t receiving_cursor_down = 0;

static struct ctimer upt;
static struct ctimer downt;

static struct unicast_conn sensor_up_unicast;
static struct unicast_conn sensor_down_unicast;

/*---------------------------------------------------------------------------*/

void send_up(void* ptr) {
    if (state == CONNECTED) {
        sensor_u* d = ups[sending_cursor_up];
        uint8_t cnt = 0;
        while (d == NULL && cnt < SENDING_QUEUE_SIZE) { 
            sending_cursor_up = (sending_cursor_up + 1) % SENDING_QUEUE_SIZE;
            d = ups[sending_cursor_up];
            cnt++;
        }
        if (d != NULL) send_sensor_message(&sensor_up_unicast, &parent, d);
    }
    
    ctimer_restart(&upt);
}

void add_up(uint8_t message, rimeaddr_t addr, uint8_t value) {
    sensor_u* d = ups[receiving_cursor_up];
    if (d != NULL) free_sensor_message(d);
    ups[receiving_cursor_up] = create_sensor_message(message, 0, receiving_cursor_up, addr, value);
    receiving_cursor_up = (receiving_cursor_up + 1) % SENDING_QUEUE_SIZE;
}

void ack_up(uint8_t seqnum, uint8_t msg, rimeaddr_t addr) {
    sensor_u* d = ups[seqnum];
    if (d != NULL && get_msg(d) == msg && my_addr_cmp(d->st->addr, addr) == 0) free_sensor_message(d);
    ups[seqnum] = NULL;
}
/*---------------------------------------------------------------------------*/

void send_down(void* ptr) {
    if (state == CONNECTED) {
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
            } else {
	            // tell parent that no more route exist
	            maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, d->st->addr, weight);
	            send_maintenance_message(&maintenance_unicast, &parent, withdraw);
	            free_maintenance_message(withdraw);
            }
        }
    } 
    
    ctimer_restart(&downt);
}

void add_down(uint8_t message, rimeaddr_t addr) {
    sensor_u* d = downs[receiving_cursor_down];
    if (d != NULL) free_sensor_message(d);
    downs[receiving_cursor_down] = create_sensor_message(message, 0, receiving_cursor_down, addr, weight);
    receiving_cursor_down = (receiving_cursor_down + 1) % SENDING_QUEUE_SIZE;
}

void ack_down(uint8_t seqnum, uint8_t msg, rimeaddr_t addr) {
    sensor_u* d = downs[seqnum];
    if (d != NULL && get_msg(d) == msg && my_addr_cmp(d->st->addr, addr) == 0) free_sensor_message(d);
    downs[seqnum] = NULL;
}

/*---------------------------------------------------------------------------*/
static void recv_down(struct unicast_conn *c, const rimeaddr_t *from)
{
    if (state != CONNECTED) return;
    
    sensor_u message;
    message.c = (char *) packetbuf_dataptr();
    
    uint8_t ack = get_ack(&message);
    uint8_t seqnum = get_seqnum(&message);
    
    // the message can only come from our parent
    if (my_addr_cmp(parent, *from) == 0) {
        uint8_t w = message.st->value;
        parent_refresh(w);
    } else {
        return;
    }
    
    uint8_t msg = get_msg(&message);
    rimeaddr_t addr = message.st->addr;
    
    // if it is an ack, we remove the packet from our sending queue
    if (ack) {
        ack_up(seqnum, msg, addr);
        return;
    }
    
    // if the message isn't for us we transfer it
    if (my_addr_cmp(addr, rimeaddr_node_addr) != 0) {
        add_down(msg, addr);
    } else {
        sensor_state = msg;
    }
    
    // send ack
    sensor_u* a = create_sensor_message(msg, ACK, seqnum, addr, weight);
    send_sensor_message(&sensor_up_unicast, from, a);
    free_sensor_message(a);
}

static void recv_up(struct unicast_conn *c, const rimeaddr_t *from)
{
    if (state != CONNECTED) return;
    
    sensor_u message;
    message.c = (char*) packetbuf_dataptr();
    
    uint8_t ack = get_ack(&message);
    uint8_t seqnum = get_seqnum(&message);
    
    // the message can not come from our parent
    if (my_addr_cmp(parent, *from) == 0) return;
    
    uint8_t msg = get_msg(&message);
    rimeaddr_t addr = message.st->addr;
    
    // if it is an ack, we remove the packet from our sending queue
    if (ack) {
        ack_down(seqnum, msg, addr);
        return;
    }
    
    // we transfer the message
    add_up(msg, addr, message.st->value);
    
    // send ack
    sensor_u* a = create_sensor_message(msg, ACK, seqnum, addr, weight);
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
	
	int send_delay = delay(DATA_D_MIN, DATA_D_MAX);
	ctimer_set(&upt, send_delay, send_up, NULL);
	ctimer_set(&downt, send_delay + send_delay / 2, send_down, NULL);
	etimer_set(&et, 2 * delay(DATA_D_MIN, DATA_D_MAX) + CLOCK_SECOND);
	
	temperature = create_sensor(15, 15);
	humidity = create_sensor(50, 50);

	while(1) {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        
        if (state == CONNECTED) {
            uint8_t new_temp = get_data(temperature);
            if (sensor_state == PERIODIC_SEND || (sensor_state == ON_CHANGE_SEND && new_temp != last_temp)) {
                // send temp
                last_temp = new_temp;
                add_up(TEMPERATURE, rimeaddr_node_addr, last_temp);
            }
            
            uint8_t new_hum = get_data(humidity);
            if (sensor_state == PERIODIC_SEND || (sensor_state == ON_CHANGE_SEND && new_hum != last_hum)) {
                // send hum
                last_hum = new_hum;
                add_up(HUMIDITY, rimeaddr_node_addr, last_hum);
            }
        }
		
		etimer_restart(&et);
	}

	PROCESS_END();
}
