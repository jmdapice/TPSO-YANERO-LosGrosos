/*
 * my_engine.h
 *
 *  Created on: 04/05/2012
 *      Author: Florencia Reales
 */

#ifndef MY_ENGINE_H_
#define MY_ENGINE_H_

    #include <stdlib.h>
	#include <stdbool.h>
	#include <memcached/engine.h>
	#include <memcached/util.h>
	#include <memcached/visibility.h>


	/*
	 * Esta es una estructura utilizada para almacenar
	 *  y respresentar un elemento almacenado en la cache
	 */
	typedef struct {
	   void* key;
	   size_t nkey;
	   void *data;
	   size_t ndata;
	   int flags;
	   bool stored;
	   rel_time_t exptime;
	}t_cache_item;


	/*
	 * Esta es una estructura personalizada?
	 * que utilizo para almacenar la configuración
	 * que me pasa memcached
	 */
	typedef struct {
	   size_t cache_max_size;
	   size_t block_size_max;
	   size_t chunk_size;
	}t_cache_config;


	/*
	 * Esta es la estructura que utilizo para
	 * representar el engine, para que memcached
	 * pueda manipularla el
	 * primer campo de esta tiene que ser ENGINE_HANDLE_V1 engine;
	 * el resto de los campos pueden ser los que querramos
	 */
	typedef struct {
		ENGINE_HANDLE_V1 engine;
		GET_SERVER_API get_server_api;
		t_cache_config config;
	} t_engine;


	// Esta funcion es la que busca memcached
	//para ejecutar cuando levanta la shared library
	MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance
	(uint64_t interface, GET_SERVER_API get_server_api,
			ENGINE_HANDLE **handle);


#endif /* MY_ENGINE_H_ */
