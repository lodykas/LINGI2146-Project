#include "tcp.h"

/*tempo*/
void main() {}

/***************
 * typedef struct {
 * 	  packet* queueP;
 * 	  int indice;
 * 	  int size;
 * 	  int to_send;
 * 	  int sent;
 * } queue;
 */
/*TODO: verifiy how to enshure thoses two functions are called*/
void server_init()
{
	uip_listen(HTONS(PORT));
}

void server_app()
{
	queue* s = (queue*)uip_conn->appstate;
	if (uip_connected())
	{
		//s->
		/*TODO: signal that connexion is established */
		return;
	}
	if (uip_acked())
	{
		s->indice++;
		s->indice %= s->size;
		s->to_send--;
		s->sent = 1;
		/*TODO: think about the next line*/
		send();
		return;
	}
	if (uip_rexmit())
	{
		send();
		return;
	}
}

void init_queue(packet* queueP, int size)
{
	queue* s = (queue*)uip_conn->appstate;
	s->queueP = queueP;
	s->indice = 0;
	s->size = size;
	s->to_send = 0;
}
/*TODO: call send periodically somewhere*/
void send()
{
	queue* s = (queue*)uip_conn->appstate;
	if (!s->to_send || !s->sent)
		return;
	uip_send(s->queueP+s->indice, sizeof(packet));
	s->sent = 0;
}

int add_to_queue(const packet* p)
{
	queue* s = (queue*)uip_conn->appstate;
	if (s->size == s->to_send)
		return 1; //overflow
	/*TODO: erase latest packet instead of refusing the new one?*/
	memcpy(s->queueP+(s->indice+s->to_send)%s->size, p, sizeof(packet));
	return 0;
}
