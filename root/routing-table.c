#include "routing-table.h"

/*----- Table managment -----------------------------------------------------*/
route_t* create_route(rimeaddr_t addr, rimeaddr_t nexthop) {
	route_t* new_route = (route_t*) malloc(sizeof(route_t));
	if (new_route == NULL) return NULL;
	new_route->addr = addr;
	new_route->nexthop = nexthop;
	new_route->next = NULL;
	return new_route;
}

int addr_cmp(rimeaddr_t a1, rimeaddr_t a2) {
	return ((uint8_t)(a1.u8[0]) - (uint8_t)(a2.u8[0])) * 256 + 
		((uint8_t)(a1.u8[1]) - (uint8_t)(a2.u8[1]));
}

void insert_route(table_t* table, rimeaddr_t addr, rimeaddr_t nexthop) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		int cmp = addr_cmp(addr, next->addr);
		if (cmp == 0) {
			next->nexthop = nexthop;
			next->shared = 0;
			return;
		} else if (cmp < 0) {
			route_t* new_route = create_route(addr, nexthop);
			new_route->next = next;
			new_route->shared = 0;
			if (previous == NULL) table->first = new_route;
			else previous->next = new_route;
			table->size++;
			return;
		}
	
		previous = next;
		next = previous->next;
	}
	
	route_t* new_route = create_route(addr, nexthop);
	new_route->shared = 0;
	if (previous == NULL) table->first = new_route;
	else previous->next = new_route;
	table->size++;
}

void delete_route(table_t* table, rimeaddr_t addr) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		int cmp = addr_cmp(addr, next->addr);
		if (cmp == 0) {
			if (previous == NULL) table->first = next->next;
			else previous->next = next->next;
			free(next);
			table->size--;
			return;
		} else if (cmp < 0) {
			return;
		}
	
		previous = next;
		next = previous->next;
	}
}

route_t* search_route(table_t* table, rimeaddr_t addr) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		int cmp = addr_cmp(addr, next->addr);
		if (cmp == 0) {
			return next;
		} else if (cmp < 0) {
			return NULL;
		}
	
		previous = next;
		next = previous->next;
	}
	
	return NULL;
}

void route_shared(table_t* table, rimeaddr_t addr) {
    route_t* route = search_route(table, addr);
    if (route != NULL) route->shared = 1;
}

route_t* next_route(table_t* table) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		if (!next->shared) return next;
		previous = next;
		next = previous->next;
	}
	
	return NULL;
}

void reset_routes(table_t* table) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		next->shared = 0;
		previous = next;
		next = previous->next;
	}
}

void flush_table(table_t* table) {
    route_t* next = table->first;
    while (next != NULL) {
        route_t* nextnext = next->next;
        free(next);
        next = nextnext;
    }
    table->size = 0;
}
