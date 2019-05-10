#include "option-message.h"

option_u* create_option_message(uint8_t msg, uint8_t value) {
	option_message_t* st = (option_message_t*) malloc(sizeof(option_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->value = value;
	
	option_u* message = (option_u*) malloc(sizeof(option_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_option_message(struct runicast_conn* runicast, rimeaddr_t* dest, option_u* message) {
    packetbuf_copyfrom(message->c, sizeof(option_message_t));
	runicast_send(runicast, dest, MAX_RETRANSMISSIONS);
}

void free_option_message(option_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
