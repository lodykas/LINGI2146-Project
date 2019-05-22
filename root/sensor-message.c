#include "sensor-message.h"

sensor_u* create_sensor_message(uint8_t msg, uint8_t ack, uint8_t seqnum, rimeaddr_t addr, uint8_t value) {
	sensor_message_t* st = (sensor_message_t*) malloc(sizeof(sensor_message_t));
	if (st == NULL) return NULL;
	st->msg = (msg % 8) * 32 + (ack % 2) * 16 + (seqnum % 16);
	st->addr = addr;
	st->value = value;
	
	sensor_u* message = (sensor_u*) malloc(sizeof(sensor_u));
	if (message == NULL) {
	    free(st);
	    return NULL;
	}
	message->st = st;
	
	return message;
}

void send_sensor_message(struct unicast_conn* unicast, const rimeaddr_t* dest, const sensor_u* message) {
    packetbuf_copyfrom(message->c, sizeof(sensor_message_t));
	unicast_send(unicast, dest);
}

void free_sensor_message(sensor_u* message) {
    if (message != NULL) free(message->st);
    free(message);
}

uint8_t get_msg(sensor_u* message) {
    if (message == NULL) return 0;
    else return message->st->msg / 32;
}

uint8_t get_ack(sensor_u* message) {
    if (message == NULL) return 0;
    else return (message->st->msg / 16) % 2;
}

uint8_t get_seqnum(sensor_u* message) {
    if (message == NULL) return 0;
    else return message->st->msg % 16;
}
