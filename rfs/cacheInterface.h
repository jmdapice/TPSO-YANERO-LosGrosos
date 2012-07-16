/*
 * cacheInterface.h
 *
 *  Created on: 12/06/2012
 *      Author: utnso
 */

#ifndef CACHEINTERFACE_H_
#define CACHEINTERFACE_H_

#include <libmemcached/memcached.h>

memcached_st* cache_crearCacheServer(char*, uint32_t);

char* cache_armarClaveServer(uint32_t);

memcached_return cache_subir_infoServer(memcached_st*,uint32_t, char*, uint16_t);

char* cache_bajar_infoServer(memcached_st*, uint32_t, size_t*, memcached_return*);

memcached_return_t cache_borrarClaveServer(memcached_st*, uint32_t);

#endif /* CACHEINTERFACE_H_ */
