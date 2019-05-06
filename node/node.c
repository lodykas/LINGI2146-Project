/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);

char *hello = "Hello";
char *groot = "I'm (g)root";
char *welcome = "Welcome";
/*---------------------------------------------------------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
    char *message = (char *)packetbuf_dataptr();
    printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
    
    if (strncmp(groot, message, 12) == 0)
    {
        printf("Root detected with address %d.%d\n", from->u8[0], from->u8[1]);
    }
    else if (strncmp(hello, message, 6) == 0) 
    {
        printf("New node detected : %d.%d\n", from->u8[0], from->u8[1]);
    }
    else if (strncmp(welcome, message, 8) == 0)
    {
        printf("Welcome received from %d.%d\n", from->u8[0], from->u8[1]);
    }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
    char *hello = "Hello";
    static struct etimer et;

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

    PROCESS_BEGIN();

    broadcast_open(&broadcast, 129, &broadcast_call);
    
    /* Send hello */
    packetbuf_copyfrom(hello, 6);
    broadcast_send(&broadcast);

    while(1) {

        /* Delay 2-4 seconds */
        etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
