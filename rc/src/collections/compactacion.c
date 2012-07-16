/*
 * compactacion.c
 *
 *  Created on: 18/05/2012
 *      Author: Florencia Reales
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "miDiccionario.h"
#include "algoritmo_reemplazo.h"
#include "logRc.h"


void juntarParticionesVacias(t_table_info* self, int pos, int cantVacios){
	int topos= pos - cantVacios +1;
	int dsdpos= pos;


	self->elements[topos].ocupado = self->elements[dsdpos].ocupado;
    self->elements[topos].bytes_inutilizados =  self->elements[dsdpos].bytes_inutilizados;
    self->elements[topos].bytes_disponibles = self->elements[dsdpos].bytes_disponibles;
    self->elements[topos].item.data = self->elements[dsdpos].item.data;

  if(self->elements[topos].ocupado == true){

	self->elements[topos].ultimaVezUsada = self->elements[dsdpos].ultimaVezUsada;
    self->elements[topos].item.exptime = self->elements[dsdpos].item.exptime;
	self->elements[topos].item.flags = self->elements[dsdpos].item.flags;
	self->elements[topos].item.nkey = self->elements[dsdpos].item.nkey;
	self->elements[topos].item.stored = self->elements[dsdpos].item.stored;
	self->elements[topos].item.ndata = self->elements[dsdpos].item.ndata;
	memcpy(self->elements[topos].item.key, self->elements[dsdpos].item.key,
			self->elements[topos].item.nkey);
  }

}


int first_fit(t_table_info* self, int size_data) {
	bool lugar = false;
	int pos = -1;
	int pos_primero = 0;
	int i, pos_bytes, cantVacios = 0;
	int free_bytes_amount = 0;

	for (i = 0; ((i < self->table_max_size) && (lugar == false)); i++) {

		if (self->elements[i].ocupado == true) {
			free_bytes_amount = 0;
			pos_primero = 0;
			cantVacios = 0;

		} else {
			if (self->elements[i].bytes_disponibles != 0) {
				free_bytes_amount += self->elements[i].bytes_disponibles;
				pos_primero++;
				cantVacios++;

				if (pos_primero == 1) {
					pos_bytes = i;
				}
			}
			//si ademas es la cantidad que necesito salgo guardando la posicion
			if (free_bytes_amount >= size_data  && free_bytes_amount >= self->size_chunk) {
				lugar = true;

				self->elements[pos_bytes].bytes_disponibles = free_bytes_amount;
				if (cantVacios > 1) {
					for (pos = pos_bytes + 1; pos <pos_bytes + cantVacios;pos++) {
						self->elements[pos].ocupado = false;
						self->elements[pos].bytes_disponibles = 0;
					}
					int tope = self->table_max_size - 1;
					for (pos = pos_bytes + cantVacios; pos < tope; pos++) {
						juntarParticionesVacias(self, pos, cantVacios);
					}
				}

				return pos_bytes;

			}
		} //fin 1er if
	}//fin del for
	return -1;
}




int best_fit(t_table_info* self, int size_data) {

	int i, pos, pos_bytes, vaciosmin;
	int posmin = -1;
	int pos_primero = 0;
	int free_bytes_amount = 0;
	int cantVacios = 0;
	int min = self->espacio_libre + 1;

	for (i = 0; i < self->table_max_size; i++) {

		if (self->elements[i].ocupado == true) {
			free_bytes_amount = 0;
			pos_primero = 0;
			cantVacios = 0;

		} else {
			if (self->elements[i].bytes_disponibles != 0) {
				free_bytes_amount += self->elements[i].bytes_disponibles;
				pos_primero++;
				cantVacios++;

				if (pos_primero == 1) {
					pos_bytes = i;
				}

				if (free_bytes_amount >= size_data && free_bytes_amount >= self->size_chunk) {
					//busco el menor hueco, que se ajuste mejor
					if (min > free_bytes_amount) {
						min = free_bytes_amount;
						vaciosmin = cantVacios;
						posmin = pos_bytes;
					}
				}
			}else{
				free_bytes_amount = 0;
						pos_primero = 0;
						cantVacios = 0;
			}

		}
	}
	if (posmin != -1) {

		self->elements[posmin].bytes_disponibles = min;
        if (vaciosmin > 1){
			for (pos = posmin + 1; pos <posmin + vaciosmin;pos++) {
				self->elements[pos].ocupado = false;
				self->elements[pos].bytes_disponibles = 0;
			}
			int tope = self->table_max_size - 1;
			for (pos = posmin + vaciosmin; pos < tope; pos++) {
				juntarParticionesVacias(self, pos, vaciosmin);
			}
        }
	}

	return posmin;

}







void* desplazarArriba (t_table_info* self, int pos, int cantVacios){

	int topos= pos;
	int dsdpos= pos + cantVacios;

	self->elements[topos].ultimaVezUsada = self->elements[dsdpos].ultimaVezUsada;
	self->elements[topos].item.exptime = self->elements[dsdpos].item.exptime;
	self->elements[topos].item.flags = self->elements[dsdpos].item.flags;
	self->elements[topos].item.nkey = self->elements[dsdpos].item.nkey;
	self->elements[topos].item.stored = self->elements[dsdpos].item.stored;
	self->elements[topos].ocupado = self->elements[dsdpos].ocupado;
 	memcpy(self->elements[topos].item.key ,self->elements[dsdpos].item.key,self->elements[topos].item.nkey);
    self->elements[topos].bytes_inutilizados =  self->elements[dsdpos].bytes_inutilizados;
    self->elements[topos].bytes_disponibles = self->elements[dsdpos].bytes_disponibles;


	int size_data= self->elements[dsdpos].item.ndata;
	self->elements[topos].item.ndata= size_data;

	if (topos == 0) {
		memmove(self->elements[topos].item.data,
				self->elements[dsdpos].item.data, size_data);
	} else {
		if (self->elements[topos - 1].item.ndata > self->size_chunk) {
			void* comienzo = self->elements[topos - 1].item.data
					+ self->elements[topos - 1].item.ndata;
			self->elements[topos].item.data = comienzo;
			memmove(comienzo, self->elements[dsdpos].item.data, size_data);
		} else {
			void* comienzo = self->elements[topos - 1].item.data
					+ self->size_chunk;
			self->elements[topos].item.data = comienzo;
			memmove(comienzo, self->elements[dsdpos].item.data, size_data);
		}
	}


	self->elements[dsdpos].ocupado= false;
    self->elements[dsdpos].bytes_inutilizados = 0;
    self->elements[dsdpos].bytes_disponibles = 0;


	return NULL;
}


bool compactar(t_table_info* self, int pos,t_log* logger,int frecuencia) {

	int cantVacios = 0;
	int new_bytes_disponibles = 0;
	int acum_bytes_disp = 0;
	int pos1 = pos;
    int posAux;

	while (self->elements[pos1].ocupado == false) {
		cantVacios++;
		acum_bytes_disp += self->elements[pos1].bytes_disponibles;
		 self->elements[pos1].bytes_disponibles=0;
		pos1++;
		if (pos1 >= self->table_max_size) {
			//tamaño de la cache - espacio libre...
			self->elements[pos].bytes_disponibles = acum_bytes_disp;
			self->elements[pos].bytes_inutilizados = 0;

			return false;
		}
	}
	int cantOcupados = 0;
	int pos2 = cantVacios + pos;
	bool salir = false;
	while (self->elements[pos2].ocupado == true && salir == false) {
		cantOcupados++;
		new_bytes_disponibles += self->elements[pos2].bytes_inutilizados;
		self->elements[pos2].bytes_inutilizados=0;
		pos2++;
		if (pos2 >= self->table_max_size) {
			if (cantVacios == 0) {
				return false;
			} else {
				salir = true;
			}
		}
	}

	int i;
	for (i = 0; i < cantOcupados; i++) {
		posAux = pos + i;
		desplazarArriba(self,posAux,cantVacios);

	}

	//cargar bytes inutilizados y disponibles
	int totalDisponibles = new_bytes_disponibles + acum_bytes_disp;
	if (totalDisponibles < self->size_chunk) {
		self->elements[posAux].bytes_inutilizados = totalDisponibles;
	} else {
	if(self->elements[posAux].item.ndata > self->size_chunk){
		self->elements[posAux + 1].item.data = self->elements[posAux].item.data+
				self->elements[posAux].item.ndata;
	}else{
		self->elements[posAux + 1].item.data = self->elements[posAux].item.data+
				self->size_chunk;
	}
		self->elements[posAux + 1].bytes_disponibles = totalDisponibles;
	}

	logRc_info(logger, "Compactando con cantidad de busquedas fallidas : %d",frecuencia);

    return true;
}


/*
 * 1. Se buscara una particion libre que tenga suficiente memoria contigua
 * como para contener el valor. En caso de no encontrarla, se pasara al paso sgte
 * (se debera poder configurar la frecuencia de compactacion[en la unidad "cantidad
 * de busquedas fallidas"] el valor -1 indicara compactar solamente cuando se hayan
 * eliminado todas las particiones, o al paso 3.
 *
 * 2. Se compactara la memoria y se realizara una nueva busqueda . En caso de
 * no encontrarla, se pasara al paso siguiente.
 *
 * 3. Se procedera a eliminar una particion de datos, y luego se volvera al paso
 * 2 0 3 segun corresponda.
 */
int compactacion(t_table_info* self, int frecuencia, int nbytes_data,
                      	char* alg_busq, char* alg_reemp, t_log* logger) {

	int lugar =-1;
	int esFifo,firstfit;
	int busquedasFallidas=0;
	int i;
	char bufftemp[100];
	t_key s_key;

	while (lugar == -1) {

		firstfit = strcmp(alg_busq, "first_fit");
		if (firstfit == 0) {
			lugar = first_fit(self, nbytes_data);
		} else {
			lugar = best_fit(self, nbytes_data);
		}

		if (lugar == -1) {
			busquedasFallidas++;
			if (busquedasFallidas == frecuencia) {
				bool fin = true;
				for (i = 0; fin == true; i++) {
					if (self->elements[i].ocupado == false) {
						fin = compactar(self, i, logger, frecuencia);
					}
				}
			} else {
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

				} else {
					bool fin = true;
					for (i = 0; fin == true; i++) {
						if (self->elements[i].ocupado == false) {
							fin = compactar(self, i, logger, frecuencia);
						}
					}
				}
			}
		}

	}

	return lugar;
}


void dump_particionesDinamicas(t_table_info* self, int table_index,FILE *arch){

 if (self->elements[table_index].item.ndata >= self->size_chunk) {

			void* fin = self->elements[table_index].item.data
					+ self->elements[table_index].item.ndata + self->elements[table_index].bytes_inutilizados - 1;
			fprintf(arch," %p .", fin);
			fprintf(arch," [X] ");
			fprintf(arch,"  Size = %i   ",	self->elements[table_index].item.ndata
							+ self->elements[table_index].bytes_inutilizados);
		} else {
			void* fin = self->elements[table_index].item.data + self->size_chunk - 1;
			fprintf(arch," %p .", fin);
			fprintf(arch," [X] ");
			fprintf(arch,"  Size = %i   ", self->size_chunk);
		}
}
