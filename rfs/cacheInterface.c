/*
 * cache.c
 *
 *  Created on: 12/06/2012
 *      Author: utnso
 */


#include "cacheInterface.h"
#include <stdio.h>
#include <libmemcached/memcached.h>


memcached_st* cache_crearCacheServer(char* ip, uint32_t port){

	memcached_st* st_cache;

	st_cache = memcached_create(NULL);

	//memcached_behavior_set( st_cache, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true);

	memcached_server_add(st_cache, ip, port);

	return st_cache;
};


memcached_return_t cache_borrarClaveServer(memcached_st* st_cache, uint32_t nroBloque){

	memcached_return_t retorno;
	char* key = cache_armarClaveServer(nroBloque);

	retorno = memcached_delete(st_cache,key,strlen(key), (time_t)0);
	free(key);
	return retorno;
};

char* cache_armarClaveServer(uint32_t nroBloque){


	char* clave = malloc(11); //10 digitos posibles del bloque + \0
	sprintf(clave,"%d",nroBloque);
	//memcpy(clave, &nroBloque, sizeof(uint32_t));

	return clave;
};

memcached_return cache_subir_infoServer(memcached_st* st_cache, uint32_t nroBloque, char* payload, uint16_t payloadLenght){

	memcached_return retorno;

	char* key = cache_armarClaveServer(nroBloque);

	retorno = memcached_set (st_cache, key, strlen(key), payload, (size_t)payloadLenght, (time_t)0, (uint32_t)0);

	free(key);

	return retorno;
};

char* cache_bajar_infoServer(memcached_st* st_cache, uint32_t nroBloque, size_t * payloadLength, memcached_return* error){

	char* payload;

	char* key = cache_armarClaveServer(nroBloque);
	uint32_t flags;

	payload = memcached_get (st_cache, key, strlen(key), payloadLength, &flags, error);

	free(key);

	return payload;
}
