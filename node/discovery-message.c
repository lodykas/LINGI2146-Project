#include "discovery-message.h"

discovery_u* create_discovery_message(uint8_t msg, uint8_t weight) {
    discovery_message_t* st = (discovery_message_t*) malloc(sizeof(discovery_message_t));
    if (st == NULL) return NULL;
    st->msg = msg;
    st->weight = weight;
    
	discovery_u* message = (discovery_u*) malloc(sizeof(discovery_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_discovery_message(struct broadcast_conn* broadcast, discovery_u* message) {
	packetbuf_copyfrom(message->c, sizeof(discovery_message_t));
	broadcast_send(broadcast);
}

void free_discovery_message(discovery_u* message) {
    if (message != NULL) free(message->st);
	free(message);
}
