#include "discovery.h"

#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "discovery-message.h"
#include "maintenance-message.h"

/*---------------------------------------------------------------------------*/
void open_discovery() {
    broadcast_open(&discovery_broadcast, 129, &discovery_callback);
    
	ctimer_set(&hellot, delay(HELLO_D_MIN, HELLO_D_MAX), send_hello, &hellot);
	ctimer_set(&welcomet, delay(WELCOME_D_MIN, WELCOME_D_MAX), send_welcome, &welcomet);
	ctimer_set(&root_checkt, delay(ROOT_CHECK_D_MIN, ROOT_CHECK_D_MAX), send_root_check, &root_checkt);
	
	ctimer_stop(&welcomet);
	ctimer_stop(&root_checkt);
}

void close_discovery() {
    broadcast_close(&discovery_broadcast);
}

void discovery_recv(struct broadcast_conn *c, const rimeaddr_t *from) {
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

void send_hello(void* ptr) {
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
