#include "routing-table.h"

LIST(routing_table);
MEMB(routing_mem, route_t, MAX_ROUTES);

void insert_route(rimeaddr_t addr, rimeaddr_t nexthop)
{
    route_t* r;
    for (r = list_head(routing_table); r != NULL; r = r->next)
    {
        if (rimeaddr_cmp(&addr, &r->addr))
        {
            r->nexthop = nexthop;
            ctimer_restart(&r->expiret);
            return;
        }
    }

    r = memb_alloc(&routing_mem);
    if (r != NULL)
    {
        r->addr = addr;
        r->nexthop = nexthop;
        timer_set(&r->sharet, 0);
        ctimer_set(&r->expiret, EXPIRE_D, delete_route, r);

        list_add(routing_table, r);
        
        printf("new route %d.%d\n", r->addr.u8[0], r->addr.u8[1]);
    }
}

void delete_route(void* ptr)
{
    route_t* r = ptr;
    printf("del route %d.%d\n", r->addr.u8[0], r->addr.u8[1]);

    list_remove(routing_table, r);
    memb_free(&routing_mem, r);
}

route_t* search_route(rimeaddr_t addr)
{
    route_t* r;
    for (r = list_head(routing_table); r != NULL; r = r->next)
    {
        if (rimeaddr_cmp(&addr, &r->addr))
            return r;
    }

    return NULL;
}

void route_shared(rimeaddr_t addr)
{
    route_t* route = search_route(addr);
    if (route != NULL)
        timer_set(&route->sharet, SHARE_D);
}

rimeaddr_t next_route()
{
    route_t* r;
    for (r = list_head(routing_table); r != NULL; r = r->next)
    {
        if (timer_expired(&r->sharet))
            return r->addr;
    }

    return rimeaddr_node_addr;
}

void reset_routes()
{
    route_t* r;
    for (r = list_head(routing_table); r != NULL; r = r->next)
    {
        timer_restart(&r->sharet);
        ctimer_restart(&r->expiret);
    }
}

void flush_table()
{
    while (list_pop(routing_table) != NULL);
}

int table_size()
{
    return list_length(routing_table);
}

char* table_to_string(char s[])
{
    sprintf(s, "1.0");
    route_t* r;
    for (r = list_head(routing_table); r != NULL; r = r->next)
    {
        sprintf(s, "%s, %d.%d", s, r->addr.u8[0], r->addr.u8[1]);
    }
    return s;
}
