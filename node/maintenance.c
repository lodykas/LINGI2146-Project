#include "maintenance.h"

#include "utils.h"
#include "discovery-message.h"
#include "maintenance-message.h"

void open_maintenance() {
    unicast_open(&maintenance_unicast, 140, &maintenance_callback);
    
    ctimer_set(&keep_alivet, delay(KEEP_ALIVE_D_MIN, KEEP_ALIVE_D_MAX), send_keep_alive, &keep_alivet);
	ctimer_set(&onlinet, delay(ONLINE_D_MIN, ONLINE_D_MAX), disconnect, &onlinet);
	ctimer_set(&routet, delay(ROUTE_D_MIN, ROUTE_D_MAX), send_route, &routet);
	
	ctimer_stop(&keep_alivet);
	ctimer_stop(&onlinet);
	ctimer_stop(&routet);
}

void close_maintenance() {
    unicast_close(&maintenance_unicast);
}

void maintenance_recv(struct unicast_conn *c, const rimeaddr_t *from)
{
    maintenance_u message;
    message.c = (char *) packetbuf_dataptr();
    uint8_t msg = message.st->msg;
    rimeaddr_t addr = message.st->addr;
    uint8_t w = message.st->weight;
	unsigned char u0 = from->u8[0];
	unsigned char u1 = from->u8[1];
    
    uint8_t parent_cmp = rimeaddr_cmp(&parent, from);
    if (parent_cmp) {
    	// update parent related infos
    	parent_refresh(w);
    } else if (state == CONNECTED) {
        // child is still reachable
	    insert_route(*from, *from);
    }
    
    switch(msg) {
        case KEEP_ALIVE:
			if (debug) printf("KEEP_ALIVE message received from %d.%d: '%d'\n", u0, u1, w);
			if (state == CONNECTED && !parent_cmp) {
	            // send welcome (if none is already in preparation)
	            if (ctimer_expired(&welcomet)) ctimer_restart(&welcomet);
			}
			break;
		case ROUTE:
			if (debug) printf("ROUTE message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
			insert_route(addr, *from);
			// send route ack
			maintenance_u* ack = create_maintenance_message(ROUTE_ACK, addr, weight);
			send_maintenance_message(&maintenance_unicast, from, ack);
			free_maintenance_message(ack);
			break;
		case ROUTE_ACK:
		    if (debug) printf("ROUTE_ACK message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);
		    // the route as been succesfully shared
			route_shared(addr);
			break;
		case ROUTE_WITHDRAW:
		    if (debug) printf("ROUTE_WITHDRAW message received from %d.%d: '%d.%d'\n", u0, u1, addr.u8[0], addr.u8[1]);			
			route_t* route = search_route(addr);
			// if route in table or if route comes from sender
			if (route != NULL && !rimeaddr_cmp(&route->nexthop, from)) {
			    delete_route((void*) route);
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

void send_keep_alive(void* ptr) {
    if (state == CONNECTED) {
        maintenance_u* message = create_maintenance_message(KEEP_ALIVE, rimeaddr_node_addr, weight);
        send_maintenance_message(&maintenance_unicast, &parent, message);
        free_maintenance_message(message);
    }
    
    ctimer_restart(&keep_alivet);
}

void send_route(void* ptr) {
    if (state == CONNECTED) {
        // send route
        rimeaddr_t addr = next_route();
        if (!rimeaddr_cmp(&rimeaddr_node_addr, &addr)) {
            maintenance_u* message = create_maintenance_message(ROUTE, addr, weight);
            send_maintenance_message(&maintenance_unicast, &parent, message);
            free_maintenance_message(message);
        }
    }
    
    ctimer_restart(&routet);
}
