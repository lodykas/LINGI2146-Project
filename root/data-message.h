#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_RETRANSMISSIONS 4

/*----- Broadcast message managment -----------------------------------------*/

void send_data_message(struct runicast_conn* runicast, rimeaddr_t* dest, char* message, uint8_t length);

