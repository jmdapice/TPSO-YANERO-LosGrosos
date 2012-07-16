/*
 * buddy.c
 *
 *  Created on: 01/07/2012
 *      Author: utnso
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "miDiccionario.h"
#include "algoritmo_reemplazo.h"
#include "logRc.h"

void buddy_create_element(t_table_info *self, const void* key, size_t size_key,
		int pos, size_t size_data) {

	memcpy(self->elements[pos].item.key, key, size_key);
	self->elements[pos].item.ndata = size_data;
	self->elements[pos].item.nkey = size_key;
	self->elements[pos].ocupado = true;
	lru_get(self, pos);
	self->elements[pos].bytes_inutilizados =
			self->elements[pos].bytes_disponibles - size_data;

	if (size_data < self->size_chunk) {
		self->espacio_libre -= self->size_chunk;

	} else {
		self->espacio_libre -= size_data
				+ self->elements[pos].bytes_inutilizados;

	}

	self->elements[pos].bytes_disponibles = 0;
	self->elements_amount++;

}


void desplazarUnoArriba(t_table_info *self,int pos_hna){
	int pos;

	for (pos = pos_hna; pos < self->table_max_size - 1; pos++) {

		self->elements[pos].ocupado = self->elements[pos + 1].ocupado;
		self->elements[pos].bytes_inutilizados =
				self->elements[pos + 1].bytes_inutilizados;
		self->elements[pos].bytes_disponibles =
				self->elements[pos + 1].bytes_disponibles;
		self->elements[pos].item.data = self->elements[pos + 1].item.data;
		if (self->elements[pos].ocupado == true) {
			self->elements[pos].ultimaVezUsada =
					self->elements[pos + 1].ultimaVezUsada;
			self->elements[pos].item.exptime =
					self->elements[pos + 1].item.exptime;
			self->elements[pos].item.flags = self->elements[pos + 1].item.flags;
			self->elements[pos].item.stored =
					self->elements[pos + 1].item.stored;

			self->elements[pos].item.ndata = self->elements[pos + 1].item.ndata;
			self->elements[pos].item.nkey = self->elements[pos + 1].item.nkey;
			memcpy(self->elements[pos].item.key,
					self->elements[pos + 1].item.key,
					self->elements[pos].item.nkey);
		}

		self->elements[pos + 1].ocupado = false;
		self->elements[pos + 1].bytes_inutilizados = 0;
		self->elements[pos + 1].bytes_disponibles = 0;
	}


}

int desplazarUnoAbajo(t_table_info *self, int posAct, int free_bytes, int ndata) {

	int i;
	int ultimaPos = self->table_max_size - 1;
	int cantADesplazar = ultimaPos - posAct + 1;

	for (i = 0; i < cantADesplazar; i++) {

		self->elements[ultimaPos].bytes_disponibles = self->elements[ultimaPos
				- 1].bytes_disponibles;
		self->elements[ultimaPos].bytes_inutilizados = self->elements[ultimaPos
				- 1].bytes_inutilizados;
		self->elements[ultimaPos].ocupado =
				self->elements[ultimaPos - 1].ocupado;
		self->elements[ultimaPos].item.data =
				self->elements[ultimaPos - 1].item.data;

		if (self->elements[ultimaPos].ocupado == true) {
			self->elements[ultimaPos].ultimaVezUsada = self->elements[ultimaPos
					- 1].ultimaVezUsada;
			self->elements[ultimaPos].item.exptime = self->elements[ultimaPos
					- 1].item.exptime;
			self->elements[ultimaPos].item.flags =
					self->elements[ultimaPos - 1].item.flags;
			self->elements[ultimaPos].item.ndata =
					self->elements[ultimaPos - 1].item.ndata;
			self->elements[ultimaPos].item.nkey =
					self->elements[ultimaPos - 1].item.nkey;
			self->elements[ultimaPos].item.stored =
					self->elements[ultimaPos - 1].item.stored;
			memcpy(self->elements[ultimaPos].item.key,
					self->elements[ultimaPos - 1].item.key,
					self->elements[ultimaPos].item.nkey);
		}
		ultimaPos--;

	}
	self->elements[posAct].bytes_disponibles = free_bytes;
	self->elements[posAct].item.data = self->elements[posAct + 1].item.data;
	self->elements[posAct + 1].ocupado = false;
	self->elements[posAct + 1].bytes_disponibles = free_bytes;
	self->elements[posAct + 1].bytes_inutilizados = 0;
	if (posAct < self->table_max_size - 1) {
		self->elements[posAct + 1].item.data = self->elements[posAct].item.data
				+ free_bytes;
	}

	return free_bytes;
}


int buscarHueco(t_table_info* self, int ndata) {

	int free_bytes = 0;
	int i;
	int part;


	for (i = 0; (i < self->table_max_size ); i++) {

		if (self->elements[i].ocupado == false) {

			free_bytes = self->elements[i].bytes_disponibles;

			if (free_bytes >= ndata) {

				part = free_bytes / 2;

				while (part >= ndata) {
					if (part >= self->size_chunk) {
						desplazarUnoAbajo(self, i, part, ndata);
						part = part / 2;
					}else{
						part=-1;
					}
				}
				return i;
			}
		}

	} //fin del for
	return -1;
}




void unirTodasLasHermanas(t_table_info* self) {

	int i;
	int acum;
	int hermana;
	bool seUnieron = true;

	while (seUnieron == true) {
		seUnieron = false;
        acum=0;
		for (i = 0; i < self->table_max_size; i++) {
			if (self->elements[i].ocupado == false
					&& self->elements[i].bytes_disponibles != 0) {
				int c = acum / self->elements[i].bytes_disponibles;
				if (c % 2 == 0) {
					//   mi hmno es el de abajo
					hermana = i + 1;
					if ((self->elements[hermana].ocupado == false)
							&& (self->elements[hermana].bytes_disponibles
									== self->elements[i].bytes_disponibles)) {
						self->elements[hermana].bytes_disponibles +=
								self->elements[i].bytes_disponibles;
						self->elements[hermana].item.data = self->elements[i].item.data;
						desplazarUnoArriba(self, i);
						seUnieron = true;

					}
					acum += self->elements[i].bytes_disponibles;

				} else {
					//si es impar mi hmno es el de arriba
					hermana = i - 1;
					if (self->elements[hermana].ocupado == false
							&& self->elements[hermana].bytes_disponibles
									== self->elements[i].bytes_disponibles) {
						acum += self->elements[i].bytes_disponibles;
						self->elements[i].bytes_disponibles +=
								self->elements[hermana].bytes_disponibles;
						self->elements[i].item.data = self->elements[hermana].item.data;
						desplazarUnoArriba(self, hermana);
						seUnieron = true;


					} else {
						acum += self->elements[i].bytes_disponibles;
					}
				}
			} else {
				if (self->elements[i].ocupado == true) {
					if (self->elements[i].item.ndata >= self->size_chunk) {
						acum += self->elements[i].item.ndata
								+ self->elements[i].bytes_inutilizados;
					} else {
						acum += self->size_chunk;
					}
				}
			}
		}
	}

}

int buddysystem(t_table_info* self, int nbytes_data, char* alg_reemp,t_log* logger ) {

	int esFifo;
	int pos = -1;
	int i;
	char bufftemp[100];
	t_key s_key;

	while (pos == -1) {

		pos = buscarHueco(self, nbytes_data);

		if (pos == -1) {
			unirTodasLasHermanas(self);
			pos = buscarHueco(self, nbytes_data);
		}

		if (pos == -1) {
			if (self->elements_amount != 0) {
				esFifo = strcmp(alg_reemp, "fifo");

				s_key = lru(self);
				char *k = (char *)(s_key.key);
				for(i=0;i<s_key.nkey;i++) {
					bufftemp[i]=k[i];
				}
				bufftemp[s_key.nkey]='\0';

				if (esFifo == 0) {

					logRc_debug(logger,
							"Algoritmo de Reemplazo: FIFO - Key: %s", bufftemp);
				} else {

					logRc_debug(logger,
							"Algoritmo de Reemplazo: LRU - Key: %s", bufftemp);
				}

			}
		}
	}

	return pos;
}

void dump_buddy(t_table_info* self, int table_index,FILE *arch){
	if (self->elements[table_index].item.ndata >= self->size_chunk) {
			//aca para el budddy tengo q sumarle los inutilizados
			void* fin = self->elements[table_index].item.data
					+ self->elements[table_index].item.ndata - 1;
			fprintf(arch," %p .", fin);
			fprintf(arch," [X] ");
			fprintf(arch,"  Size = %i   ",self->elements[table_index].item.ndata
							+ self->elements[table_index].bytes_inutilizados);
		} else {
			void* fin = self->elements[table_index].item.data
					+ self->size_chunk - 1;
			fprintf(arch," %p .", fin);
			fprintf(arch," [X] ");
			fprintf(arch,"  Size = %i   ", self->size_chunk);
		}

}
