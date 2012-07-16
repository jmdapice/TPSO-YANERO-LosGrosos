/*
 * auxCliente.h
 *
 *  Created on: 11/06/2012
 *      Author: utnso
 */

#ifndef AUXCLIENTE_H_
#define AUXCLIENTE_H_

#include "fsc.h"
#include "../commons/collections/queue.h"
#include "cacheInterface.h"

typedef struct {
	char *pathDir;
	char *nombreArch;
} PathSeparado;

t_socket_client* obtenerSocket(t_queue*);

void habilitar_socket(t_queue*, t_socket_client*);

void levantar_config(int*, int*, char**, char**, int*, char**, int*);

void borrarCacheDirectorios(char*, memcached_st*);

PathSeparado separarPathCliente(char*);

#endif /* AUXCLIENTE_H_ */
