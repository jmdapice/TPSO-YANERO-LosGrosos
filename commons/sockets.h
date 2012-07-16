/*
 * socket.h
 *
 *  Created on: 28/05/2012
 *      Author: utnso
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CON 100
#define HEADERSIZE 3
#define HANDSHAKECLIENT "Groso Connect"
#define HANDSHAKESERV "Groso Ok"

typedef enum {
	SOCKETSTATE_CONNECTED,
	SOCKETSTATE_DISCONNECTED
}e_socket_state;

typedef struct {
	int desc;
	struct sockaddr_in* my_addr;
}t_socket;

typedef struct {
	t_socket* socket;
	e_socket_state state;
}t_socket_client;

typedef struct {
	t_socket *socket;
	int maxconexiones;
}t_socket_server;

t_socket_client *sockets_createClient(char *ip, int32_t port);
int 		     sockets_connect(t_socket_client *cliente,char *ip, int32_t port);
int 			 sockets_send(t_socket_client *cliente,void *data, int dataLen);
void 			 sockets_destroy_client(t_socket_client *cliente);
t_socket_server *sockets_createServer(char *ip, int port);
int				 sockets_listen(t_socket_server *server);
t_socket_client *sockets_accept(t_socket_server *server);
void			 sockets_destroyServer(t_socket_server* server);
void			*sockets_createBuffer(size_t bufflen);
void			 sockets_destroyBuffer(void *self);
int				 sockets_recvHeaderNipc(t_socket_client *self, void *buffer);
int				 sockets_recvPayloadNipc(t_socket_client *self, void *buffer, uint16_t payloadLength);
char 			 enviarHandshake(t_socket_client *self); //fsc
char			 recibirHandshake(t_socket_client *self); //rfs


#endif /* SOCKET_H_ */
