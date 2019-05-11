#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

// TODO add timestamp for shared and updated
struct route {
	rimeaddr_t addr;
	rimeaddr_t nexthop;
	uint8_t shared;
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

int addr_cmp(rimeaddr_t a1, rimeaddr_t a2);

void insert_route(table_t* table, rimeaddr_t addr, rimeaddr_t nexthop);

void delete_route(table_t* table, rimeaddr_t addr);

route_t* search_route(table_t* table, rimeaddr_t addr);

void route_shared(table_t* table, rimeaddr_t addr);

route_t* next_route(table_t* table);

void reset_routes(table_t* table);

void flush_table(table_t* table);
