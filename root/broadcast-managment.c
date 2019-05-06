#include "broadcast-managment.h"

discovery_t* create_message(uint8_t msg, uint8_t weight) {
	discovery_struct_t* st = (discovery_struct_t*) malloc(sizeof(discovery_struct_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->weight = weight;
	
	discovery_t* message = (discovery_t*) malloc(sizeof(discovery_t));
	if (message == NULL) return NULL;
	message->st = st;
	
	return message;
}

void send_message(struct broadcast_conn* broadcast, discovery_t* message) {
	packetbuf_copyfrom(message->c, sizeof(discovery_struct_t));
	broadcast_send(broadcast);
}

void free_message(discovery_t* message) {
	if (message->st != NULL) free(message->st);
	free(message);
}
