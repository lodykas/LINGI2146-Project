#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_ROUTES 64

#define SHARE_D 600 * CLOCK_SECOND
#define EXPIRE_D 1800 * CLOCK_SECOND

struct route
{
    struct route* next;
    rimeaddr_t addr;
    rimeaddr_t nexthop;
    struct timer sharet;
    struct ctimer expiret;
};
typedef struct route route_t;

/*----- Table management -----------------------------------------------------*/
route_t* create_route(rimeaddr_t addr, rimeaddr_t nexthop);

void insert_route(rimeaddr_t addr, rimeaddr_t nexthop);

void delete_route(void* ptr);

route_t* search_route(rimeaddr_t addr);

void route_shared(rimeaddr_t addr);

rimeaddr_t next_route();

void reset_routes();

void flush_table();
