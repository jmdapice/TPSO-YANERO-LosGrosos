/*
 * sockets.c
 *
 *  Created on: 03/06/2012
 *      Author: utnso
 */

#include "sockets.h"
#include <stdio.h>

t_socket_client *sockets_createClient(char *ip, int32_t port) {

	t_socket *sock;
	t_socket_client *self;
	int on=1;
	uint32_t addrlen = sizeof(struct sockaddr_in);
	int fd;
	struct sockaddr_in *dir_local; //Dir mia

	sock = (t_socket *)malloc(sizeof(t_socket));
	self = (t_socket_client *)malloc(sizeof(t_socket_client));
	dir_local=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

	fd = socket(AF_INET,SOCK_STREAM,0);
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))==-1){
		perror("setsockopt");
		exit(0);
	}

	dir_local->sin_addr.s_addr = inet_addr(ip);
	dir_local->sin_family = AF_INET;
	dir_local->sin_port = htons(port);

	if(bind(fd,(struct sockaddr *)dir_local,addrlen)==-1){
		perror("bind");
		exit(0);
	}

	sock->desc = fd;
	sock->my_addr = dir_local;
	self->socket = sock;
	self->state = SOCKETSTATE_DISCONNECTED;

	return self;

}

int sockets_connect(t_socket_client *cliente,char *ip, int32_t port) {

	int connectResult;
	uint32_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in dir_remota;//Dir del server

	dir_remota.sin_addr.s_addr = inet_addr(ip); // Esta struct la armo estatica porque es solo para pasarla al connect
	dir_remota.sin_family = AF_INET;
	dir_remota.sin_port = htons(port);
	connectResult = connect(cliente->socket->desc, (struct sockaddr *) &dir_remota,addrlen);
	if(connectResult != -1) {
		cliente->state = SOCKETSTATE_CONNECTED;
	}

	return connectResult;
}

int sockets_send(t_socket_client *cliente,void *data, int dataLen) {

	return send(cliente->socket->desc,data,dataLen,MSG_WAITALL);
}

void sockets_destroy_client(t_socket_client *cliente) {

	close(cliente->socket->desc);
	free(cliente->socket->my_addr);
	free(cliente->socket);
	free(cliente);
}

t_socket_server *sockets_createServer(char *ip, int port) {

	t_socket *sock;
	t_socket_server *self;
	int on=1;
	uint32_t addrlen = sizeof(struct sockaddr_in);
	int fd;
	struct sockaddr_in *dir_local; //Dir mia

	sock = (t_socket *)malloc(sizeof(t_socket));
	self = (t_socket_server *)malloc(sizeof(t_socket_server));
	dir_local=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

	fd = socket(AF_INET,SOCK_STREAM,0);
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))==-1){
		perror("setsockopt");
		exit(0);
	}

	dir_local->sin_addr.s_addr = inet_addr(ip);
	dir_local->sin_family = AF_INET;
	dir_local->sin_port = htons(port);

	if(bind(fd,(struct sockaddr *)dir_local,addrlen)==-1){
		perror("error bind");
		exit(0);
	}

	sock->desc = fd;
	sock->my_addr = dir_local;
	self->socket = sock;
	self->maxconexiones = MAX_CON;

	return self;

}

int sockets_listen(t_socket_server *server) {

	return listen(server->socket->desc,server->maxconexiones);
}

t_socket_client *sockets_accept(t_socket_server *server) {

	t_socket *sock;
	t_socket_client *cliente;
	uint32_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in *dir_remota;
	sock = (t_socket *)malloc(sizeof(t_socket));
	cliente = (t_socket_client *)malloc(sizeof(t_socket_client));
	dir_remota=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

	sock->desc = accept(server->socket->desc,(struct sockaddr *) dir_remota, &addrlen);
	sock->my_addr = dir_remota;
	if(sock->desc == -1) {
		return NULL;
	} else {
		cliente->socket = sock;
		cliente->state = SOCKETSTATE_CONNECTED;
	}
	return cliente;
}

void *sockets_createBuffer(size_t bufflen) {

	void *self;
	self = malloc(bufflen);
	return self;
}

void sockets_destroyBuffer(void *self) {

	free(self);
}

int sockets_recvHeaderNipc(t_socket_client *self, void *buffer) {

	return recv(self->socket->desc,buffer,HEADERSIZE,MSG_WAITALL);
}

int sockets_recvPayloadNipc(t_socket_client *self, void *buffer, uint16_t payloadLength) {

	return recv(self->socket->desc,buffer,payloadLength,MSG_WAITALL);
}

void sockets_destroyServer(t_socket_server* server) {

	close(server->socket->desc);
	free(server->socket->my_addr);
	free(server->socket);
	free(server);

}


char recibirHandshake(t_socket_client *self) { //esta funcion es para el rfs

	char* buffer;
	int32_t recvd;
	char result;
	buffer = sockets_createBuffer(strlen(HANDSHAKECLIENT)+1);
	recvd = recv(self->socket->desc,buffer,strlen(HANDSHAKECLIENT)+1,MSG_WAITALL);
	if(recvd <= 0) result = -1;
	if(!(strcmp(buffer,HANDSHAKECLIENT))) {
		sockets_send(self,HANDSHAKESERV,strlen(HANDSHAKESERV)+1);
		result = 1;
	} else {
		result = 0;
	}
	sockets_destroyBuffer(buffer);
	return result;
}

char enviarHandshake(t_socket_client *self) { //esta funcion es para el fsc

	char* buffer;
	int32_t recvd;
	char result;
	sockets_send(self,HANDSHAKECLIENT,strlen(HANDSHAKECLIENT)+1);
	buffer = sockets_createBuffer(strlen(HANDSHAKESERV)+1);
	recvd = recv(self->socket->desc,buffer,strlen(HANDSHAKESERV)+1,MSG_WAITALL);
	if(recvd <= 0) result = -1;
	if(!(strcmp(buffer,HANDSHAKESERV))) {
		result = 1;
	} else {
		result = 0;
	}
	sockets_destroyBuffer(buffer);
	return result;
}



