#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "discovery.h"
#include "maintenance.h"
#include "data.h"
#include "sensor-data.h"

/*---------------------------------------------------------------------------*/
PROCESS(node_tree, "Routing tree discovery");
PROCESS(node_data, "Data transfer");
AUTOSTART_PROCESSES(&node_tree, &node_data);

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(node_tree, ev, data)
{
    static struct etimer et;

    PROCESS_EXITHANDLER(
        close_discovery();
        close_maintenance();
    )

    PROCESS_BEGIN();

    debug = 0;

    parent = rimeaddr_node_addr;
    weight = MAX_WEIGHT;
    state = OFFLINE;

    open_discovery();
    open_maintenance();

    etimer_set(&et, CLOCK_SECOND);

    while(1)
    {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        etimer_restart(&et);
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* DATA sharing                                                              */
/*---------------------------------------------------------------------------*/
sensor_data_t* temperature;
sensor_data_t* humidity;

uint8_t last_temp = 0;
uint8_t last_hum = 0;

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(node_data, ev, data)
{
    static struct etimer et;

    PROCESS_EXITHANDLER(
        data_close();
    )

    PROCESS_BEGIN();

    data_open();

    sensor_state = ON_CHANGE_SEND;

    etimer_set(&et, 2 * delay(DATA_D_MIN, DATA_D_MAX) + CLOCK_SECOND);

    temperature = create_sensor(15, 15);
    humidity = create_sensor(50, 50);

    while(1)
    {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        if (state == CONNECTED)
        {
            uint8_t new_temp = get_data(temperature);
            if (sensor_state == PERIODIC_SEND || (sensor_state == ON_CHANGE_SEND && new_temp != last_temp))
            {
                // send temp
                last_temp = new_temp;
                add_up(TEMPERATURE, rimeaddr_node_addr, last_temp);
            }

            uint8_t new_hum = get_data(humidity);
            if (sensor_state == PERIODIC_SEND || (sensor_state == ON_CHANGE_SEND && new_hum != last_hum))
            {
                // send hum
                last_hum = new_hum;
                add_up(HUMIDITY, rimeaddr_node_addr, last_hum);
            }
        }

        etimer_restart(&et);
    }

    PROCESS_END();
}
