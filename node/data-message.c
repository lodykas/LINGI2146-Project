#include "data-message.h"

data_u* create_data_message(uint8_t msg, rimeaddr_t addr, uint8_t value) {
	data_message_t* st = (data_message_t*) malloc(sizeof(data_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->addr = addr;
	st->value = value;
	
	data_u* message = (data_u*) malloc(sizeof(data_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_data_message(struct runicast_conn* runicast, rimeaddr_t* dest, data_u* message) {
    packetbuf_copyfrom(message->c, sizeof(data_message_t));
	runicast_send(runicast, dest, MAX_RETRANSMISSIONS);
}

void free_data_message(data_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
