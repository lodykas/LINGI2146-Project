#include "broadcast-message.h"

discovery_t* create_broadcast_message(uint8_t msg, uint8_t weight) {
	discovery_struct_t* st = (discovery_struct_t*) malloc(sizeof(discovery_struct_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->weight = weight;
	
	discovery_t* message = (discovery_t*) malloc(sizeof(discovery_t));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_broadcast_message(struct broadcast_conn* broadcast, discovery_t* message) {
	packetbuf_copyfrom(message->c, sizeof(discovery_struct_t));
	broadcast_send(broadcast);
}

void free_broadcast_message(discovery_t* message) {
	if (message != NULL) free(message->st);
	free(message);
}
