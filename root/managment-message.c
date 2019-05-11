#include "managment-message.h"

manage_u* create_manage_message(uint8_t msg, rimeaddr_t addr) {
	manage_message_t* st = (manage_message_t*) malloc(sizeof(manage_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->addr = addr;
	
	manage_u* message = (manage_u*) malloc(sizeof(manage_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_manage_message(struct runicast_conn* runicast, rimeaddr_t* dest, manage_u* message) {
    packetbuf_copyfrom(message->c, sizeof(manage_message_t));
	runicast_send(runicast, dest, MAX_RETRANSMISSIONS);
}

void free_manage_message(manage_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
