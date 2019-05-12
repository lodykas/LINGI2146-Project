#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#define SHARE_D 600
#define EXPIRE_D 1800

static const unsigned long delay_share = SHARE_D * CLOCK_SECOND;
static const unsigned long delay_expire = EXPIRE_D * CLOCK_SECOND;

struct route {
	rimeaddr_t addr;
	rimeaddr_t nexthop;
	struct timer sharet;
	struct timer expiret;
	struct route* next;
};
typedef struct route route_t;

struct table {
	route_t* first;
	unsigned int size;
};
typedef struct table table_t;

/*----- Table managment -----------------------------------------------------*/
route_t* create_route(rimeaddr_t addr, rimeaddr_t nexthop);

int my_addr_cmp(rimeaddr_t a1, rimeaddr_t a2);

void insert_route(table_t* table, rimeaddr_t addr, rimeaddr_t nexthop);

void delete_route(table_t* table, rimeaddr_t addr);

route_t* search_route(table_t* table, rimeaddr_t addr);

void route_shared(table_t* table, rimeaddr_t addr);

route_t* next_route(table_t* table);

void reset_routes(table_t* table);

void flush_table(table_t* table);
