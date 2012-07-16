/*
 * algoritmo_reemplazo.c
 *
 *  Created on: 18/05/2012
 *      Author: Florencia Reales
 */



#include <stdio.h>
#include <time.h>
#include <string.h>
#include "algoritmo_reemplazo.h"
#define BILLION  1000000000L;


t_key lru(t_table_info *self){

	void* key_victim;
	int i, size_key;
	t_key s_key;
	struct timespec time;
	double tiempoActual, min_time;
	s_key.key = NULL;
	s_key.nkey = 0;

	clock_gettime(1, &time);
	tiempoActual = (time.tv_sec) + (double) (time.tv_nsec) / (double) BILLION;
	min_time = tiempoActual;

	for (i = 0; i < self->table_max_size; i++) {
		if (self->elements[i].ocupado == true) {
			if (self->elements[i].item.stored == true) {
				if (self->elements[i].ultimaVezUsada < min_time) {
					min_time = self->elements[i].ultimaVezUsada;
					size_key = self->elements[i].item.nkey;
				    key_victim = self->elements[i].item.key;
				}
			}
		}
	}
	//Eliminarlo
	micache_remove(self, key_victim, size_key);
	s_key.key = key_victim;
	s_key.nkey = size_key;
	return s_key;


}



void lru_get(t_table_info *self, int pos) {

	struct timespec time;
	double tiempoActual;

	clock_gettime(1, &time);
	tiempoActual = (time.tv_sec) + (double) (time.tv_nsec) / (double) BILLION;

	self->elements[pos].ultimaVezUsada = (double) tiempoActual;

}
