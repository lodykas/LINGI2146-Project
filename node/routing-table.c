#include "routing-table.h"

/*----- Table managment -----------------------------------------------------*/
route_t* create_route(rimeaddr_t addr, rimeaddr_t nexthop) {
	route_t* new_route = (route_t*) malloc(sizeof(route_t));
	if (new_route == NULL) return NULL;
	new_route->addr = addr;
	new_route->nexthop = nexthop;
	timer_set(&(new_route->sharet), 0);
	timer_set(&(new_route->expiret), delay_expire);
	new_route->next = NULL;
	return new_route;
}

int my_addr_cmp(rimeaddr_t a1, rimeaddr_t a2) {
	return ((int)(a1.u8[1]) - (int)(a2.u8[1])) * 256 + 
		((int)(a1.u8[0]) - (int)(a2.u8[0]));
}

// update timestamp update
void insert_route(table_t* table, rimeaddr_t addr, rimeaddr_t nexthop) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		int cmp = my_addr_cmp(addr, next->addr);
		if (cmp == 0) {
			next->nexthop = nexthop;
			timer_restart(&(next->expiret));
			return;
		} else if (cmp < 0) {
			route_t* new_route = create_route(addr, nexthop);
			new_route->next = next;
			if (previous == NULL) table->first = new_route;
			else previous->next = new_route;
			table->size++;
			return;
		}
	
		previous = next;
		next = previous->next;
	}
	
	route_t* new_route = create_route(addr, nexthop);
	if (previous == NULL) table->first = new_route;
	else previous->next = new_route;
	table->size++;
}

void delete_route(table_t* table, rimeaddr_t addr) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		int cmp = my_addr_cmp(addr, next->addr);
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
		int cmp = my_addr_cmp(addr, next->addr);
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

// update timestamp shared
void route_shared(table_t* table, rimeaddr_t addr) {
    route_t* route = search_route(table, addr);
    if (route != NULL) timer_set(&(route->sharet), delay_share);
}

// tenir compte des timestamp shared and update
route_t* next_route(table_t* table) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
	    if (timer_expired(&(next->expiret))) {
	        if (my_addr_cmp(rimeaddr_node_addr, next->addr) == 0) {
	            timer_restart(&(next->expiret));
	        } else {
	            printf("Route expired : %d.%d\n", next->addr.u8[0], next->addr.u8[1]);
	            if (previous == NULL) table->first = next->next;
			    else previous->next = next->next;
			    free(next);
			    table->size--;
			}
	    } else if (timer_expired(&(next->sharet))) {
	        return next;
	    }
		previous = next;
		next = previous->next;
	}
	
	return NULL;
}

// reset timestamp shared
void reset_routes(table_t* table) {
	route_t* previous = NULL;
	route_t* next = table->first;
	while (next != NULL) {
		timer_set(&(next->sharet), 0);
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
