/*
 * my_engine.c
 *
 *  Created on: 04/05/2012
 *      Author: Florencia Reales
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <mcheck.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>


// Aca estan las tools de memcached para levantar la configuración provista
//por el usuario en los parametros de ejecución
#include <memcached/config_parser.h>
#include <memcached/protocol_binary.h>
#include <memcached/engine.h>
#include "collections/miDiccionario.h"
#include "collections/buddy.h"
#include "collections/compactacion.h"
#include "collections/algoritmo_reemplazo.h"
#include "my_engine.h"
#include "../../commons/config.h"
#include "collections/logRc.h"

/*
 * Estas son las funciones estaticas necesarias para que el engine funcione
 */

static ENGINE_ERROR_CODE cache_initialize(ENGINE_HANDLE* , const char* config_str);
static void destroy_engine(ENGINE_HANDLE*, const bool force);

static ENGINE_ERROR_CODE item_allocate(ENGINE_HANDLE* , const void* cookie, item **item, const void* key,
											const size_t nkey, const size_t nbytes, const int flags, const rel_time_t exptime);
static bool get_item_info(ENGINE_HANDLE *, const void *cookie, const item* item, item_info *item_info);
static ENGINE_ERROR_CODE store_info(ENGINE_HANDLE* , const void *cookie, item* item, uint64_t *cas,
											ENGINE_STORE_OPERATION operation, uint16_t vbucket);
static void item_release(ENGINE_HANDLE* , const void *cookie, item* item);
static ENGINE_ERROR_CODE get_info(ENGINE_HANDLE* , const void* cookie, item** item, const void* key, const int nkey, uint16_t vbucket);

static ENGINE_ERROR_CODE flush_all(ENGINE_HANDLE* , const void* cookie, time_t when);

static ENGINE_ERROR_CODE delete_one(ENGINE_HANDLE* , const void* cookie, const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket);

/*
 * ************************** Dummy Functions **************************
 *
 * Estas funciones son dummy, son necesarias para que el engine las tengas
 * pero no tienen logica alguna y no seran necesarias implementar
 *
 */

static const engine_info* console_info(ENGINE_HANDLE* );
static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* , const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat);
static void dummy_ng_reset_stats(ENGINE_HANDLE* , const void *cookie);
static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* , const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response);
static void dummy_ng_item_set_cas(ENGINE_HANDLE *, const void *cookie, item* item, uint64_t val);

/**********************************************************************/


/*
 * Esta función es la que va a ser llamada cuando se reciba una signal
 */
void dummy_ng_dummp(int signal);


/* Declaración de variables globales*/

t_table_info *cache;
pthread_mutex_t sem_cache=PTHREAD_MUTEX_INITIALIZER;
t_log *logger;
pthread_mutex_t sem_log=PTHREAD_MUTEX_INITIALIZER;
t_log_level log_level;
char* esquema;
char* alg_busqueda;
char* alg_reemplazo;
int frecuenciaCompactacion;
void *ptrdata;

/*
 * Esta es la función que va a llamar memcached
 *  para instanciar nuestro engine
 */
MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance
(uint64_t interface, GET_SERVER_API get_server_api, ENGINE_HANDLE **handle) {

	/*
	 * Verify that the interface from the server is one we support. Right now
	 * there is only one interface, so we would accept all of them (and it would
	 * be up to the server to refuse us... I'm adding the test here so you
	 * get the picture..
	 */
	if (interface == 0) {
		return ENGINE_ENOTSUP;
	}

	/*
	 * Allocate memory for the engine descriptor. I'm no big fan of using
	 * global variables, because that might create problems later on if
	 * we later on decide to create multiple instances of the same engine.
	 * Better to be on the safe side from day one...
	 */
	t_engine *engine = calloc(1, sizeof(t_engine));
	if (engine == NULL) {
		return ENGINE_ENOMEM;
	}

	/*
	 * We're going to implement the first version of the engine API, so
	 * we need to inform the memcached core what kind of structure it should
	 * expect
	 */
	engine->engine.interface.interface = 1;

	/*
	 * La API de memcache funciona pasandole a la estructura engine
	 * que esta dentro de nuestra
	 * estructura t_engine los punteros a las funciónes necesarias.
	 */
	engine->engine.initialize = cache_initialize;
	engine->engine.destroy = destroy_engine;
	engine->engine.get_info = console_info;
	engine->engine.allocate = item_allocate;
	engine->engine.remove = delete_one;
	engine->engine.release = item_release;
	engine->engine.get = get_info;
	engine->engine.get_stats = dummy_ng_get_stats;
	engine->engine.reset_stats = dummy_ng_reset_stats;
	engine->engine.store = store_info;
	engine->engine.flush = flush_all;
	engine->engine.unknown_command = dummy_ng_unknown_command;
	engine->engine.item_set_cas = dummy_ng_item_set_cas;
	engine->engine.get_item_info = get_item_info;
	engine->get_server_api = get_server_api;

	/*
	 * memcached solo sabe manejar la estructura ENGINE_HANDLE
	 * el cual es el primer campo de nuestro t_engine
	 * El puntero de engine es igual a &engine->engine
	 *
	 * Retornamos nuestro engine a traves de la variable handler
	 */
	*handle = (ENGINE_HANDLE*) engine;

	/* creo la cache de almacenamiento */

	void _cache_item_destroy(void *item){
		// La variable item es un elemento que esta
		// dentro del dictionary, el cual es un
		// t_cache_item. Este solo puede ser borrado
		// si no esta "storeado"

		((t_cache_item*)item)->stored = false;

		item_release(NULL, NULL, item);
	}

	cache = micache_create(_cache_item_destroy);

	return ENGINE_SUCCESS;
}



/*
 * Esta función se llama inmediatamente despues del create_instance
 *  y sirve para inicializar la cache.
 */
static ENGINE_ERROR_CODE cache_initialize(ENGINE_HANDLE* handle,
		                                    const char* config_str){

	t_engine *engine = (t_engine*)handle;

	char *log_level_str;


	/* se levanta el archivo de configuración */
		t_config *s_cfg;

		s_cfg = config_create("configRc.txt");
		if (config_has_property(s_cfg, "esqAdmMemoria")) {
			  esquema = config_get_string_value(s_cfg, "esqAdmMemoria");
		} else {
			puts("Faltan campos en el 1");
			exit(0);
		}
		if (config_has_property(s_cfg, "cantBusquedasFallidas")) {
			  frecuenciaCompactacion = config_get_int_value(s_cfg, "cantBusquedasFallidas");
		} else {
			puts("Faltan campos en el 2 ");
			exit(0);
		}

		if (config_has_property(s_cfg, "algoritmobusqueda")) {
		    alg_busqueda = config_get_string_value(s_cfg, "algoritmobusqueda");
		} else {
			puts("Faltan campos en el 3");
			exit(0);
		}
		if (config_has_property(s_cfg, "algoritmoreemplazo")) {
			 alg_reemplazo = config_get_string_value(s_cfg, "algoritmoreemplazo");
			} else {
				puts("Faltan campos en el 4");
				exit(0);
		}
		if(config_has_property(s_cfg,"log")) {
			log_level_str = config_get_string_value(s_cfg,"log");
			log_level = logRc_level_from_stringRc(log_level_str);
		}else{
			puts("Faltan campos en el 5");
			exit(0);
		}

		/* se crea el log */

		logger = logRc_create("logRc.txt","rc",true,log_level);
	/*
	 * La función parse_config recibe este string y una estructura
	 *  que contienelas keys que debe saber interpretar, los tipos
	 *  y los punteros de las variables donde tiene
	 * que almacenar la información.
	 *
	 */
	if (config_str != NULL) {
	  struct config_item items[] = {
		 { .key = "cache_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.cache_max_size },
		 { .key = "chunk_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.chunk_size },
		 { .key = "item_size_max",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.block_size_max },
		 { .key = NULL}
	  };

		parse_config(config_str, items, NULL);
	}

    //alocar memoria de las estructuras necesarias para la cache

	int size_Max_cache = engine->config.block_size_max;
	int size_min_chunk = engine->config.chunk_size;
    int totalNodos = (size_Max_cache / size_min_chunk);
    pthread_mutex_lock(&sem_cache);
	cache->table_max_size = totalNodos;
	cache->espacio_libre = size_Max_cache;
	cache->size_chunk = size_min_chunk;

	setenv("MALLOC_TRACE", "output", 1);
	mtrace();

	const void* desde = malloc(1);
	mlock(desde, 111000);
	void *ptrkey = malloc(61*totalNodos);
	void *ptrdata = malloc(size_Max_cache);
	cache->elements = (t_array*) malloc(cache->table_max_size *sizeof(t_array));


	int i;
	for (i = 0; i < totalNodos; i++) {
		cache->elements[i].ocupado = false;
	    cache->elements[i].item.key = ptrkey;
		ptrkey += 60;
	}

	cache->elements[0].item.data = ptrdata;
	cache->elements[0].bytes_disponibles = size_Max_cache;
    cache->elements[0].bytes_inutilizados = 0;

    pthread_mutex_unlock(&sem_cache);

	/*
	 * Registro la SIGUSR1. El registro de signals
	 * debe ser realizado en la función initialize
	 */
	signal(SIGUSR1, dummy_ng_dummp);

	return ENGINE_SUCCESS;
}


/*
 * Esta función es la que se llama cuando el engine es destruido
*/
static void destroy_engine(ENGINE_HANDLE* handle, const bool force){
	//free(handle);
}

/*
 * Esto retorna algo de información la cual se muestra en la consola
 */
static const engine_info* console_info(ENGINE_HANDLE* handle) {
	static engine_info info = {
	          .description = "Mi Engine versión 0.1",
	          .num_features = 0,
	          .features = {
	               [0].feature = ENGINE_FEATURE_LRU,
	               [0].description = "No hay soporte de LRU"
	           }
	};

	return &info;
}

// Esta función se encarga de alocar un item.
//Este item es la metadata necesaria para almacenar la key
// y el valor. Esta función solo se llama temporalemente antes de hacer,
//por ejemplo, un store. Luego del store
// el motor llama a la función release. Es por ello que utilizamos
//un flag "stored" para saber si el elemento
// alocado realmente fue almacenado en la cache o no.
// Puede ocurrir que este mismo se llame 2 veces para la misma operación.
//Esto es porque el protocolo ASCII de
// memcached hace el envio de la información en 2 partes, una con la key,
//size y metadata y otra parte con la data en si.
// Por lo que es posible que una vez se llame para hacer
//un alocamiento temporal del item y luego se llame otra vez, la cual
// si va a ser almacenada.


static ENGINE_ERROR_CODE item_allocate(ENGINE_HANDLE *handler, const void* cookie,
		                                item **item, const void* key,
										const size_t nkey, const size_t nbytes,
										const int flags, const rel_time_t exptime){



	t_engine *engine = (t_engine*)handler;
	int pdata;
	int particionesdinamicas = strcmp(esquema, "particionesdinamicas");
	int i;
	char* k= (char*)key;
	char bufftemp[100];
	for(i=0;i<nkey;i++) {
		bufftemp[i]=k[i];
	}
	bufftemp[nkey]='\0';


	pthread_mutex_lock(&sem_log);
	logRc_debug(logger, "Operacion SET - Key: %s",bufftemp);
	pthread_mutex_unlock(&sem_log);

	if (nbytes > engine->config.block_size_max ) {
			return ENGINE_ENOMEM;
		}

	pthread_mutex_lock(&sem_cache);
    int estaGuardado = micache_search_element(cache, key ,nkey);
	if ( estaGuardado != -1){
		micache_remove(cache,key,nkey);
	}
	if (particionesdinamicas == 0) {
		pthread_mutex_lock(&sem_log);
		pdata = compactacion(cache, frecuenciaCompactacion, nbytes,
				alg_busqueda, alg_reemplazo, logger);
		pthread_mutex_unlock(&sem_log);
	} else {
		pthread_mutex_lock(&sem_log);
		pdata = buddysystem(cache, nbytes, alg_reemplazo, logger);
		pthread_mutex_unlock(&sem_log);
	}
	pthread_mutex_unlock(&sem_cache);

	if (pdata == -1) {
		return ENGINE_ENOMEM;
	}

	pthread_mutex_lock(&sem_cache);
	if (particionesdinamicas == 0) {
		micache_create_element(cache, key, nkey, pdata, nbytes,
				engine->config.chunk_size);
	} else {
		buddy_create_element(cache, key, nkey, pdata, nbytes);
	}
	*item = &cache->elements[pdata].item;
	pthread_mutex_unlock(&sem_cache);


	return ENGINE_SUCCESS;
}

/*
 * Destruye un item alocado, esta es una función que tiene
 * que utilizar internamente memcached
 */
static void item_release(ENGINE_HANDLE *handler, const void *cookie,
                                                    item* item){
	t_cache_item* it_aux = (t_cache_item*)item;

	pthread_mutex_lock(&sem_cache);
	if (!it_aux->stored) {
		micache_remove(cache,it_aux->key,it_aux->nkey);
	}
	pthread_mutex_unlock(&sem_cache);

}

/*
 * Esta función lo que hace es mapear el item_info
 * el cual es el tipo que memcached sabe manejar con el tipo de item
 * nuestro el cual es el que nosotros manejamos
 */
static bool get_item_info(ENGINE_HANDLE *handler, const void *cookie, const item* item, item_info *item_info){
	// casteamos de item*, el cual es la forma
	//generica en la cual memcached  a nuestro tipo de item, al tipo
	// correspondiente que nosotros utilizamos

	t_cache_item* it = (t_cache_item*)item;

	if (item_info->nvalue < 1) {
	  return false;
	}

	item_info->cas = 0; 		/* Not supported */
	item_info->clsid = 0; 		/* Not supported */
	item_info->exptime = it->exptime;
	item_info->flags = it->flags;
	item_info->key = it->key;
	item_info->nkey = it->nkey;
	item_info->nbytes = it->ndata; 	/* Total length of the items data */
	item_info->nvalue = 1; 			/* Number of fragments used ( Default ) */
	item_info->value[0].iov_base = it->data; /* Hacemos apuntar item_info al comienzo de la info */
	item_info->value[0].iov_len = it->ndata; /* Le seteamos al item_info el tamaño de la información */

	return true;
}

/*
 * Esta funcion es invocada cuando memcached recibe el comando get
 */
static ENGINE_ERROR_CODE get_info(ENGINE_HANDLE *handle,
		const void* cookie, item** item, const void* key,
		const int nkey, uint16_t vbucket){

	int i;
	char* k= (char*)key;
	char bufftemp[100];
	for(i=0;i<nkey;i++) {
		bufftemp[i]=k[i];
	}
	bufftemp[nkey]='\0';

	pthread_mutex_lock(&sem_log);
	logRc_debug(logger, "Operacion GET - Key: %s",bufftemp);
	pthread_mutex_unlock(&sem_log);

	// buscamos y obtenemos las pos del item
	int pos = micache_search_element(cache, key ,nkey);

	if (pos == -1) {
		return ENGINE_KEY_ENOENT;
	}

	int lru = strcmp(alg_reemplazo, "lru");
    if (lru == 0){
    	pthread_mutex_lock(&sem_cache);
    	lru_get(cache,pos);
        pthread_mutex_unlock(&sem_cache);
    }

    pthread_mutex_lock(&sem_cache);
    *item = &cache->elements[pos].item;
     pthread_mutex_unlock(&sem_cache);

	return ENGINE_SUCCESS;

}

/*
 * Esta función se llama cuando memcached recibe un set.
 * La variable operation nos indica el tipo.
 * Estos deben ser tratados indistintamente
 */
static ENGINE_ERROR_CODE store_info(ENGINE_HANDLE *handle, const void *cookie,
		item* item, uint64_t *cas, ENGINE_STORE_OPERATION operation,
		uint16_t vbucket) {

	t_cache_item *it = (t_cache_item*)item;

 	pthread_mutex_lock(&sem_cache);
	it->stored = true;
	pthread_mutex_unlock(&sem_cache);


	*cas = 0;
	return ENGINE_SUCCESS;
}

/*
 * Esta función se llama cuando memcached recibe un flush_all
 */
static ENGINE_ERROR_CODE flush_all(ENGINE_HANDLE* handle, const void* cookie, time_t when) {

	pthread_mutex_lock(&sem_cache);
	micache_clean(cache);
	pthread_mutex_unlock(&sem_cache);

	return ENGINE_SUCCESS;
}

/*
 * Esta función se llama cuando memcached recibe un delete
 */
static ENGINE_ERROR_CODE delete_one(ENGINE_HANDLE* handle, const void* cookie,
		const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket) {

    int pos = micache_search_element(cache,key,nkey);

	if (pos == -1) {
		return ENGINE_KEY_ENOENT;
	}

    t_cache_item *item = &cache->elements[pos].item;
    item->stored = false;
	item_release(handle, NULL, item);


	return ENGINE_SUCCESS;
}

/*
 * ************************************* Funciones Dummy *************************************
 */

static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* handle, const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat) {
	return ENGINE_SUCCESS;
}

static void dummy_ng_reset_stats(ENGINE_HANDLE* handle, const void *cookie) {

}

static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* handle, const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response) {
	return ENGINE_ENOTSUP;
}

static void dummy_ng_item_set_cas(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t val) {

}


/*
 * Handler de la SIGUSR1
 */
void dummy_ng_dummp(int signal){


	int buddy = strcmp(esquema,"particionesdinamicas");
    int lru = strcmp(alg_reemplazo, "lru");
    pthread_mutex_lock(&sem_cache);
    micache_iterator(cache, buddy,lru);
	pthread_mutex_unlock(&sem_cache);
}
