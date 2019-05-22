#include "utils.h"

#include "discovery-message.h"
#include "maintenance-message.h"
#include "sensor-message.h"

#include "sensor-data.h"

void choose_parent(rimeaddr_t addr, uint8_t w) {
    if (w == MAX_WEIGHT) weight = MAX_WEIGHT;
    else weight = w + 1;
    parent = addr;
	state = CONNECTED;
	
	printf("New parent chosen : %d.%d\n", parent.u8[0], parent.u8[1]);
	
	ctimer_stop(&hellot);
	ctimer_restart(&keep_alivet);
	ctimer_restart(&onlinet);
	ctimer_restart(&routet);
	ctimer_restart(&upt);
	ctimer_restart(&downt);
	
	reset_routes();
}

void disconnect(void* ptr) {
    printf("Disconnecting...\n");
	state = OFFLINE;
	weight = MAX_WEIGHT;
	parent = rimeaddr_node_addr;
	
	ctimer_restart(&hellot);
	ctimer_stop(&keep_alivet);
	ctimer_stop(&onlinet);
	ctimer_stop(&routet);
	ctimer_stop(&upt);
	ctimer_stop(&downt);
	
	flush_table();
}

void parent_refresh(uint8_t w) {
    if (w == MAX_WEIGHT) weight = MAX_WEIGHT;
    else weight = w + 1;
    ctimer_restart(&keep_alivet);
    ctimer_restart(&onlinet);
}

unsigned int delay(unsigned int min, unsigned int max) {
    uint8_t u0 = rimeaddr_node_addr.u8[0];
    uint8_t u1 = rimeaddr_node_addr.u8[1];
    unsigned int var = ((u1 * CLOCK_SECOND) + u0 * 17 + u1);
    return min * CLOCK_SECOND + var % ((max - min) * CLOCK_SECOND);
}

