/*
 * miDiccionario.c
 *
 *  Created on: 09/05/2012
 *      Author: Florencia Reales
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "miDiccionario.h"
#include "compactacion.h"
#include "buddy.h"
#include "algoritmo_reemplazo.h"





/*
 * @NAME: micache_create
 * @DESC: Crea la tabla para administrar la cache
 */

t_table_info *micache_create(void(*data_destroyer)(void*)) {

	t_table_info *self = (t_table_info*)malloc(sizeof(t_table_info));
	self->data_destroyer = data_destroyer;
	self->elements_amount = 0;
	return self;
}


/*
 * @NAME: micache_alocar
 * @DESC: Devuelve la posicion donde se puede alocar lo recibido.
*/
int micache_alocar(t_table_info *self, size_t nbytes_data, int min_size_block,
		          int(funcion)(t_table_info*,int,int,char*,char*),int frecuencia,
		          char *alg_busq, char* alg_reemp){

	int pos, firstfit;

	//Voy a buscar espacio libre continuo del tamaño del data
	// segun algoritmo a implementar first_fit o best_fit

	firstfit = strcmp(alg_busq, "first_fit");
	if (firstfit == 0) {
		pos = first_fit(self, nbytes_data);
	} else {
		pos = best_fit(self, nbytes_data);
	}

	if (pos == -1){
    	// implementar compactación o buddy system
      pos = funcion(self,frecuencia,nbytes_data,alg_busq,alg_reemp);
    }

    return pos;
}


/*
 * @NAME: micache_remove
 * @DESC: Remueve un elemento y retorna la data.
*/

void *micache_remove(t_table_info *self, const void *key, size_t size_key) {

	void *data = micache_remove_element(self, key,size_key);
	if( data != NULL){
		self->elements_amount--;
	}
	return data;
}



/*
 * @NAME: micache_iterator
 * @DESC: Hace el dump de la cache.
*/
void micache_iterator(t_table_info* self,int esBuddy,int lru) {
	int table_index;
	FILE *arch;
	arch = fopen("dumpCache.txt","a");

	// para compactacion tengo que imprimir los lugares vacios , bytes inutilizables.
	fprintf (arch,"\n--------------------------------------------------------------------------------\n");
	time_t tiempo=time(0);
	struct tm *t_local=localtime(&tiempo);
	char output[128];
	strftime(output,128,"%d/%m/%y %H:%M:%S",t_local);
	fprintf (arch,"Dump: %s\n",output);

	for (table_index = 0; table_index < self->table_max_size; table_index++) {
		//si tiene bytes disp o si esta ocupado
		if ((self->elements[table_index].bytes_disponibles != 0)
				|| (self->elements[table_index].ocupado == true)) {

			t_cache_item item = self->elements[table_index].item;
			fprintf(arch,"Partición %i", table_index);
			fprintf(arch,": %p -", self->elements[table_index].item.data);
			//si esta ocupado
			if (self->elements[table_index].ocupado == true && item.stored == true) {
               if(esBuddy != 0){
            	   dump_buddy(self,table_index,arch);
               }else{
            	   dump_particionesDinamicas(self,table_index,arch);
               }
               if(lru == 0){
            	   fprintf(arch," LRU: %.3f  ",self->elements[table_index].ultimaVezUsada);
               }else{
            	   fprintf(arch," FIFO: %.3f  ",self->elements[table_index].ultimaVezUsada);
               }
				char* k = (char*) item.key;


				int i;
				fprintf(arch," Key: ");
				for(i=0; i< item.nkey;i++){
					fprintf(arch,"%c",k[i]);
				}
				fprintf(arch,"\n");


			} else {
				void* fin = self->elements[table_index].item.data
						+ self->elements[table_index].bytes_disponibles - 1;
				fprintf(arch," %p .", fin);
				fprintf(arch," [L] ");
				fprintf(arch,"  Size = %i  ",
						self->elements[table_index].bytes_disponibles);
				fprintf(arch,"\n");
			}
		}

	}

	fprintf (arch,"\n--------------------------------------------------------------------------------\n");
	fclose(arch);
}

/*
 * @NAME: micache_clean
 * @DESC: Borra todos los elementos de la cache
*/
void micache_clean(t_table_info *self) {
	int table_index;
    int espacioLibre = 0;
	for (table_index = 0; table_index < self->table_max_size; table_index++) {
		if(self->elements[table_index].ocupado == true){

		self->elements[table_index].ocupado = false;
		self->elements[table_index].ultimaVezUsada = 0;
		if(self->elements[table_index].item.ndata >= self->size_chunk){
		espacioLibre += self->elements[table_index].item.ndata
						+ self->elements[table_index].bytes_inutilizados;
		}else{
			espacioLibre += self->size_chunk;
		}
		self->elements[table_index].item.ndata = 0;
		}else{
			espacioLibre += self->elements[table_index].bytes_disponibles
							+ self->elements[table_index].bytes_inutilizados;
		}

		self->elements[table_index].bytes_disponibles = 0;
		self->elements[table_index].bytes_inutilizados = 0;
	}
	self->elements[0].bytes_disponibles = espacioLibre;
	self->elements[0].bytes_inutilizados = 0;
    self->espacio_libre = espacioLibre;
	self->elements_amount = 0;


}



void desplazarAbajo(t_table_info *self,int pos, int disponibles){

	int i;
	int ultimaPos= self->table_max_size-1;
	int cantADesplazar = ultimaPos - pos ;

	for (i=0 ; i < cantADesplazar ; i++){

		self->elements[ultimaPos].bytes_disponibles = self->elements[ultimaPos - 1].bytes_disponibles;
        self->elements[ultimaPos].bytes_inutilizados = self->elements[ultimaPos - 1].bytes_inutilizados;
        self->elements[ultimaPos].item.data = self->elements[ultimaPos - 1].item.data;
        self->elements[ultimaPos].item.exptime = self->elements[ultimaPos - 1].item.exptime;
        self->elements[ultimaPos].item.flags = self->elements[ultimaPos - 1].item.flags;
        self->elements[ultimaPos].item.ndata = self->elements[ultimaPos - 1].item.ndata;
        self->elements[ultimaPos].item.nkey = self->elements[ultimaPos - 1].item.nkey;
        self->elements[ultimaPos].item.stored = self->elements[ultimaPos - 1].item.stored;
        memcpy(self->elements[ultimaPos].item.key,self->elements[ultimaPos - 1].item.key,self->elements[ultimaPos].item.nkey);
        self->elements[ultimaPos].ocupado = self->elements[ultimaPos - 1].ocupado;
        self->elements[ultimaPos].ultimaVezUsada = self->elements[ultimaPos - 1].ultimaVezUsada;

        ultimaPos --;


	}

	self->elements[pos].ocupado = false;
	self->elements[pos].bytes_disponibles = disponibles;
	self->elements[pos].bytes_inutilizados = 0;
	if (self->elements[pos - 1].item.ndata < self->size_chunk) {
		self->elements[pos].item.data = self->elements[pos - 1].item.data
				+ self->size_chunk;
	} else {
		self->elements[pos].item.data = self->elements[pos - 1].item.data
				+ self->elements[pos - 1].item.ndata;
	}


}




void *micache_create_element(t_table_info* self,const void* key, size_t size_key,
	 int pos, size_t size_data, int size_chunk) {


	memcpy(self->elements[pos].item.key, key, size_key);
	self->elements[pos].item.ndata = size_data;
	self->elements[pos].item.nkey = size_key;
	self->elements[pos].ocupado = true;
	lru_get(self,pos);


	if (self->elements[pos].bytes_disponibles >= size_data) {

		if (self->elements[pos + 1].ocupado == true) {
			int disponibles;
			if (size_data < size_chunk) {
				self->espacio_libre -= size_chunk;
				disponibles = self->elements[pos].bytes_disponibles - size_chunk;
			} else {
				self->espacio_libre -= size_data;
				disponibles = self->elements[pos].bytes_disponibles - size_data;
			}
			if (disponibles >= size_chunk) {
				desplazarAbajo(self, pos+1, disponibles);

			} else {
				self->elements[pos].bytes_inutilizados = disponibles;
			}

		} else { //la posicion siguiente del array esta vacia
			int disponibles;
			if (size_data < size_chunk) {
				self->espacio_libre -= size_chunk;
				disponibles = self->elements[pos].bytes_disponibles
						- size_chunk;
				self->elements[pos + 1].item.data =
						self->elements[pos].item.data + size_chunk;
			} else {
				self->espacio_libre -= size_data;
				disponibles = self->elements[pos].bytes_disponibles - size_data;

				self->elements[pos + 1].item.data =
						self->elements[pos].item.data + size_data;

			}
			if (disponibles >= size_chunk) {
				self->elements[pos + 1].bytes_disponibles += disponibles;
			} else {
				self->elements[pos].bytes_inutilizados = disponibles;
			}

		}
	} //else{ return error;}


	self->elements[pos].bytes_disponibles = 0;
	self->elements_amount++;


	return NULL;
}


 int micache_search_element(t_table_info *self,const void* key, size_t size_key){

	int i;
	for (i = 0; i < self->table_max_size; i++) {

		if (self->elements[i].ocupado == true) {
			if (self->elements[i].item.stored == true) {
				t_cache_item item = self->elements[i].item;
				// COMPARAR LAS KEYS
				if (item.nkey == size_key) {
					int thesame = memcmp(key, item.key, size_key);
					if (thesame == 0) {
						return i;
					}
				}
			}
		}
	}
		//(si no lo encuentra)
		return -1;


}


 void *micache_remove_element(t_table_info *self,  const void *key, size_t size_key) {
    int i;
    int pos;

	for (i = 0; i < self->table_max_size; i++) {

			if (self->elements[i].ocupado == true) {
					if (self->elements[i].item.nkey == size_key) {
						int thesame = memcmp(key, self->elements[i].item.key, size_key);
						if (thesame == 0) {
							pos= i;
						}
					}

			}
	}

	void* data = self->elements[pos].item.data;
	self->elements[pos].ocupado = false;
	self->elements[pos].item.stored = false;
	self->elements[pos].ultimaVezUsada = 0;

	if (self->elements[pos].item.ndata  < self->size_chunk) {
				self->elements[pos].bytes_disponibles= self->size_chunk;
			} else {
				self->elements[pos].bytes_disponibles =
						self->elements[pos].item.ndata +
						self->elements[pos].bytes_inutilizados;

			}
 self->espacio_libre+= self->elements[pos].bytes_disponibles;
 self->elements[pos].bytes_inutilizados = 0;
 self->elements[pos].item.ndata = 0;

 return data;

}


