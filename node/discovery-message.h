#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

/*----- Broadcast message management -----------------------------------------*/
struct discovery_message
{
    uint8_t msg;
    uint8_t weight;
};
typedef struct discovery_message discovery_message_t;

union discovery_union
{
    char* c;
    discovery_message_t* st;
};
typedef union discovery_union discovery_u;

discovery_u* create_discovery_message(uint8_t msg, uint8_t weight);

void send_discovery_message(struct broadcast_conn* broadcast, discovery_u* message);

void free_discovery_message(discovery_u* message);
