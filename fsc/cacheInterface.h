/*
 * cache.h
 *
 *  Created on: 12/06/2012
 *      Author: utnso
 */

#ifndef CACHEINTERFACE_H_
#define CACHEINTERFACE_H_

#include <libmemcached/memcached.h>

memcached_st* cache_crearCache(char*, uint32_t);

char* cache_armarClave (uint8_t, char*);

memcached_return cache_subir_info(memcached_st*, uint8_t, char*, char*, uint16_t);

char* cache_bajar_info(memcached_st*, uint8_t, char*, size_t*, memcached_return*);

memcached_return_t cache_borrarClave(memcached_st*, uint8_t, char*);

memcached_return_t cache_existeClave(memcached_st*, uint8_t, char*);

#endif /* CACHE_H_ */
