/*
 * miDiccionario.h
 *
 *  Created on: 09/05/2012
 *      Author: Florencia Reales
 */

#ifndef MIDICCIONARIO_H_
#define MIDICCIONARIO_H_


	#include <stdbool.h>
    #include <stdlib.h>
    #include "../my_engine.h"


    typedef struct {
        t_cache_item item;
    	bool ocupado;
    	int bytes_disponibles;
    	int bytes_inutilizados;
    	double ultimaVezUsada;
    } t_array;

	typedef struct {
		t_array* elements; //Array de particiones
		void (*data_destroyer)(void*);
		int table_max_size;
		int elements_amount;
		int espacio_libre;
		int size_chunk;
	} t_table_info;


	t_table_info *micache_create(void(*data_destroyer)(void*));
	int 		  micache_alocar(t_table_info *self, size_t nbytes_data, int min_size_block,
			      int(funcion)(t_table_info*,int,int,char*,char*),int frecuencia,char* alg_busq,char* alg_reemp);
	int 		  micache_get(t_table_info *self,int min_size_block,int pos);
	void 		 *micache_remove(t_table_info *self, const void *key, size_t size_key);
	void 		  micache_iterator(t_table_info *self,int buddy,int lru);
	void 		  micache_clean(t_table_info *self);
	void 		 *micache_create_element(t_table_info *self,const void* key,size_t size_key,int pos,
			      size_t size_data, int size);
    int           micache_search_element(t_table_info *self, const void *key,size_t size_key);
    void         *micache_remove_element(t_table_info *self, const void *key, size_t size_key);


#endif /* MIDICCIONARIO_H_ */
