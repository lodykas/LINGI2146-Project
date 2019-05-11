#include "maintenance-message.h"

maintenance_u* create_maintenance_message(uint8_t msg, uint8_t weight) {
	maintenance_message_t* st = (maintenance_message_t*) malloc(sizeof(maintenance_message_t));
	if (st == NULL) return NULL;
	st->msg = msg;
	st->weight = weight;
	
	maintenance_u* message = (maintenance_u*) malloc(sizeof(maintenance_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_maintenance_message(struct unicast_conn* unicast, const rimeaddr_t* dest, maintenance_u* message) {
    packetbuf_copyfrom(message->c, sizeof(maintenance_message_t));
	unicast_send(unicast, dest);
}

void free_maintenance_message(maintenance_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}
