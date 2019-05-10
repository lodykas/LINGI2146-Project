#include "routing-message.h"

void send_data_message(struct runicast_conn* runicast, rimeaddr_t* dest, char* message, uint8_t length) {
    packetbuf_copyfrom(message, length);
	runicast_send(runicast, dest, MAX_RETRANSMISSIONS);
}
