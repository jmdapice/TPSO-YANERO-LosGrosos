/*
 * cache.c
 *
 *  Created on: 12/06/2012
 *      Author: utnso
 */


#include "cacheInterface.h"
#include <libmemcached/memcached.h>


memcached_st* cache_crearCache(char* ip, uint32_t port){

	memcached_st* st_cache;

	st_cache = memcached_create(NULL);

	//memcached_behavior_set( st_cache, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true);

	memcached_server_add(st_cache, ip, port);

	return st_cache;
};

char* cache_armarClave (uint8_t tipoOp, char* path){

	char* clave = malloc(sizeof(uint8_t) + strlen(path) + 1);

	memcpy(clave,&tipoOp,sizeof(uint8_t));
	memcpy(clave+sizeof(uint8_t), path, strlen(path) + 1);

	return clave;
};

memcached_return cache_subir_info(memcached_st* st_cache, uint8_t tipoOp, char* path, char* payload, uint16_t payloadLenght){

	memcached_return retorno;

	char* key = cache_armarClave(tipoOp, path);

	retorno =  memcached_set (st_cache, key, strlen(key), payload, (size_t)payloadLenght, (time_t)0, (uint32_t)0);

	free(key);

	return retorno;
};

char* cache_bajar_info(memcached_st* st_cache, uint8_t tipoOp, char* path, size_t * payloadLength, memcached_return* error){

	char* payload;

	char* key = cache_armarClave(tipoOp, path);
	uint32_t flags;

	payload = memcached_get (st_cache, key, strlen(key), payloadLength, &flags, error);

	free(key);

	return payload;
}

memcached_return_t cache_borrarClave(memcached_st* st_cache, uint8_t tipoOp, char* path){

	char* key = cache_armarClave(tipoOp, path);

	return memcached_delete(st_cache,key,strlen(key), (time_t)0);

};

memcached_return_t cache_existeClave(memcached_st* st_cache, uint8_t tipoOp, char* path){

	memcached_return_t resultado;
	size_t payloadLength = 0;

	char* key = cache_armarClave(tipoOp, path);

	memcached_get (st_cache, key, strlen(key), &payloadLength, (uint32_t)0, &resultado);

	return resultado;

};


