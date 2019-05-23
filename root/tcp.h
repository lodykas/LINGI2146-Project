#define UIP_APPCALL server_app       
#define UIP_APPSTATE_SIZE sizeof(queue)
#define PORT 1234

#include <string.h>

typedef struct {
	int i;
} packet;

typedef struct {
	packet* queueP;
	int indice;
	int size;
	int to_send;
	int sent;
} queue;

void server_init();

void server_app();

void init_queue(packet* queue, int size);

void send();

int add_to_queue(const packet* p);
