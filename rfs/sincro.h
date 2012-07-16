/*
 * sincro.h
 *
 *  Created on: 26/06/2012
 *      Author: utnso
 */

#ifndef SINCRO_H_
#define SINCRO_H_

#include <pthread.h>
#include "../commons/collections/list.h"
#include <stdint.h>
#include <stdlib.h>
#include "Rfs.h"

typedef struct{
	uint32_t nroInodo;
	uint32_t cantHilos;
	pthread_rwlock_t mt_inodo;
}t_nodoInodo;


extern t_list* listaInodos;
extern pthread_mutex_t mt_lista_inodos;

void Sincro_crearMonitor(t_list* listaInodos);
void Sincro_agregarALista(uint32_t,t_list*);
void Sincro_monitorLock(uint32_t, t_list*,uint8_t);
void Sincro_monitorUnlock(uint32_t, t_list*);


#endif /* SINCRO_H_ */
