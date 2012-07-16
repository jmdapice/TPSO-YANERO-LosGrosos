/*
 * algoritmo_reemplazo.h
 *
 *  Created on: 17/05/2012
 *      Author: Florencia Reales
 */

#ifndef ALGORITMO_REEMPLAZO_H_
#define ALGORITMO_REEMPLAZO_H_

#include <stdbool.h>

#include "miDiccionario.h"

typedef struct {
	void *key;
	size_t nkey;
} t_key;

t_key lru(t_table_info *self);
void lru_get(t_table_info *self,int pos);

#endif /* ALGORITMO_REEMPLAZO_H_ */
