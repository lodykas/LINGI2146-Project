#include "routing-message.h"

routing_u* create_routing_message(uint8_t msg, rimeaddr_t addr) {
	routing_message_t* st = (routing_message_t*) malloc(sizeof(routing_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->addr = addr;
	
	routing_u* message = (routing_u*) malloc(sizeof(routing_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_routing_message(struct runicast_conn* runicast, rimeaddr_t* dest, routing_u* message) {
    packetbuf_copyfrom(message->c, sizeof(routing_message_t));
	runicast_send(runicast, dest, MAX_RETRANSMISSIONS);
}

void free_routing_message(routing_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
