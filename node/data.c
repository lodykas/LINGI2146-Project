#include "data.h"

#include "maintenance.h"
#include "maintenance-message.h"
#include "utils.h"

void data_open() {
	unicast_open(&sensor_up_unicast, 162, &sensor_up_callbacks);
	unicast_open(&sensor_down_unicast, 173, &sensor_down_callbacks);
	
	int send_delay = delay(DATA_D_MIN, DATA_D_MAX);
	ctimer_set(&upt, send_delay, send_up, NULL);
	ctimer_set(&downt, send_delay + send_delay / 2, send_down, NULL);
}

void data_close() {
	unicast_close(&sensor_up_unicast);
	unicast_close(&sensor_down_unicast);
}
/*---------------------------------------------------------------------------*/

void send_up(void* ptr) {
    if (state == CONNECTED) {
        sensor_u* d = ups[sending_cursor_up];
        uint8_t cnt = 0;
        while (d == NULL && cnt < SENDING_QUEUE_SIZE) { 
            sending_cursor_up = (sending_cursor_up + 1) % SENDING_QUEUE_SIZE;
            d = ups[sending_cursor_up];
            cnt++;
        }
        if (d != NULL) send_sensor_message(&sensor_up_unicast, &parent, d);
        sending_cursor_up = (sending_cursor_up + 1) % SENDING_QUEUE_SIZE;
    }
    
    ctimer_restart(&upt);
}

void add_up(uint8_t message, rimeaddr_t addr, uint8_t value) {
    sensor_u* d = ups[receiving_cursor_up];
    if (d != NULL) free_sensor_message(d);
    ups[receiving_cursor_up] = create_sensor_message(message, 0, receiving_cursor_up, addr, value);
    receiving_cursor_up = (receiving_cursor_up + 1) % SENDING_QUEUE_SIZE;
}

void ack_up(uint8_t seqnum, uint8_t msg, rimeaddr_t addr) {
    sensor_u* d = ups[seqnum];
    if (d != NULL && get_msg(d) == msg && my_addr_cmp(d->st->addr, addr) == 0) free_sensor_message(d);
    ups[seqnum] = NULL;
}
/*---------------------------------------------------------------------------*/

void send_down(void* ptr) {
    if (state == CONNECTED) {
        sensor_u* d = downs[sending_cursor_down];
        uint8_t cnt = 0;
        while (d == NULL && cnt < SENDING_QUEUE_SIZE) { 
            d = downs[sending_cursor_down];
            cnt++;
        }
        if (d != NULL) {
            route_t* r = search_route(&routes, d->st->addr);
            if (r != NULL) {
                send_sensor_message(&sensor_down_unicast, &(r->nexthop), d);
            } else {
	            // tell parent that no more route exist
	            maintenance_u* withdraw = create_maintenance_message(ROUTE_WITHDRAW, d->st->addr, weight);
	            send_maintenance_message(&maintenance_unicast, &parent, withdraw);
	            free_maintenance_message(withdraw);
            }
        }
        sending_cursor_down = (sending_cursor_down + 1) % SENDING_QUEUE_SIZE;
    } 
    
    ctimer_restart(&downt);
}

void add_down(uint8_t message, rimeaddr_t addr) {
    sensor_u* d = downs[receiving_cursor_down];
    if (d != NULL) free_sensor_message(d);
    downs[receiving_cursor_down] = create_sensor_message(message, 0, receiving_cursor_down, addr, weight);
    receiving_cursor_down = (receiving_cursor_down + 1) % SENDING_QUEUE_SIZE;
}

void ack_down(uint8_t seqnum, uint8_t msg, rimeaddr_t addr) {
    sensor_u* d = downs[seqnum];
    if (d != NULL && get_msg(d) == msg && my_addr_cmp(d->st->addr, addr) == 0) free_sensor_message(d);
    downs[seqnum] = NULL;
}

/*---------------------------------------------------------------------------*/
void recv_down(struct unicast_conn *c, const rimeaddr_t *from) {
    if (state != CONNECTED) return;
    
    sensor_u message;
    message.c = (char *) packetbuf_dataptr();
    
    uint8_t ack = get_ack(&message);
    uint8_t seqnum = get_seqnum(&message);
    
    // the message can only come from our parent
    if (my_addr_cmp(parent, *from) == 0) {
        uint8_t w = message.st->value;
        parent_refresh(w);
    } else {
        return;
    }
    
    uint8_t msg = get_msg(&message);
    rimeaddr_t addr = message.st->addr;
    
    // if it is an ack, we remove the packet from our sending queue
    if (ack) {
        ack_up(seqnum, msg, addr);
        return;
    }
    
    // if the message isn't for us we transfer it
    if (my_addr_cmp(addr, rimeaddr_node_addr) != 0) {
        add_down(msg, addr);
    } else {
        sensor_state = msg;
    }
    
    // send ack
    sensor_u* a = create_sensor_message(msg, ACK, seqnum, addr, weight);
    send_sensor_message(&sensor_up_unicast, from, a);
    free_sensor_message(a);
}

void recv_up(struct unicast_conn *c, const rimeaddr_t *from) {
    if (state != CONNECTED) return;
    
    sensor_u message;
    message.c = (char*) packetbuf_dataptr();
    
    uint8_t ack = get_ack(&message);
    uint8_t seqnum = get_seqnum(&message);
    
    // the message can not come from our parent
    if (my_addr_cmp(parent, *from) == 0) return;
    
    uint8_t msg = get_msg(&message);
    rimeaddr_t addr = message.st->addr;
    
    // if it is an ack, we remove the packet from our sending queue
    if (ack) {
        ack_down(seqnum, msg, addr);
        return;
    }
    
    // we transfer the message
    add_up(msg, addr, message.st->value);
    
    // send ack
    sensor_u* a = create_sensor_message(msg, ACK, seqnum, addr, weight);
    send_sensor_message(&sensor_down_unicast, from, a);
    free_sensor_message(a);
}

