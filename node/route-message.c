#include "route-message.h"

route_u* create_route_message(uint8_t msg, rimeaddr_t addr) {
	route_message_t* st = (route_message_t*) malloc(sizeof(route_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->addr = addr;
	
	route_u* message = (route_u*) malloc(sizeof(route_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_route_message(struct unicast_conn* unicast, const rimeaddr_t* dest, route_u* message) {
    packetbuf_copyfrom(message->c, sizeof(route_message_t));
	unicast_send(unicast, dest);
}

void free_route_message(route_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
