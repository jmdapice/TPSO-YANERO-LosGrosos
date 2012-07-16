#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "fsc.h"
#include "serializar_y_deserializar.h"
#include "../commons/sockets.h"
#include "../commons/log.h"
#include "../commons/config.h"
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "auxCliente.h"
#include "cacheInterface.h"
#include "../commons/collections/list.h"


/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE
 */
struct t_runtime_options {
	char* welcome_msg;
} runtime_options;


/*
 * Esta Macro sirve para definir nuestros propios parametros que queremos que
 * FUSE interprete. Esta va a ser utilizada mas abajo para completar el campos
 * welcome_msg de la variable runtime_options
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

t_log *logger;
pthread_mutex_t s_vectorPool = PTHREAD_MUTEX_INITIALIZER;
sem_t socketDisp;
pthread_mutex_t s_cache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_log = PTHREAD_MUTEX_INITIALIZER;
t_queue* pool_sock;
memcached_st* st_cache;


static int fsc_getattr(const char *path, struct stat *stbuf) {


	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	char* header = malloc(tamano_header);
	char* paquete_recv;
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	DesAttr_resp attr;
	uint8_t tipo_recv;
	t_socket_client* sock;

	size_t tamPaqCache;
	memcached_return retorno;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	memset(stbuf,0,sizeof(struct stat));

	pthread_mutex_lock(&s_cache);
	paquete_recv = cache_bajar_info(st_cache, 'a', path_, &tamPaqCache, &retorno);
	pthread_mutex_unlock(&s_cache);

	if (retorno != MEMCACHED_SUCCESS) {

		pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación GETATTR - Desde el server - Path %s", path);
		pthread_mutex_unlock(&s_log);

		paquete_send = serializar_path(path_,TIPOGETATTR);

		sem_wait(&socketDisp);
		pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
		pthread_mutex_unlock(&s_vectorPool);

		sockets_send(sock,paquete_send,tamano_paq);

		free(paquete_send);

		sockets_recvHeaderNipc(sock,header);

		memcpy(&tipo_recv,header,sizeof(uint8_t));

		if (tipo_recv != TIPOGETATTR) {
			pthread_mutex_lock(&s_vectorPool);
				habilitar_socket(pool_sock,sock);
			pthread_mutex_unlock(&s_vectorPool);

			sem_post(&socketDisp);

			pthread_mutex_lock(&s_log);
				log_debug(logger, "Error Operación GETATTR - Path %s", path);
			pthread_mutex_unlock(&s_log);

			free(header);
			free(path_);

			return -ENOENT;
		}

		memcpy(&tamano_paq,header+1,sizeof(uint16_t));
		paquete_recv = malloc(tamano_paq);
		sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);


		pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock,sock);
		pthread_mutex_unlock(&s_vectorPool);
		sem_post(&socketDisp);

		pthread_mutex_lock(&s_cache);
		retorno = cache_subir_info(st_cache, 'a', path_, paquete_recv, tamano_paq);
		pthread_mutex_unlock(&s_cache);

		free(header);

	} else {

		pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación GETATTR - Desde la cache - Path %s", path);
		pthread_mutex_unlock(&s_log);

	}

	free(path_);

	attr = deserializar_Gettattr_Rta(paquete_recv);

	stbuf->st_mode = attr.modo;
	stbuf->st_nlink = attr.nlink;
	stbuf->st_size = attr.total_size;

	pthread_mutex_lock(&s_log);
	log_debug(logger, "Fin Operación GETATTR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(paquete_recv);

	return 0;
}

static int fsc_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;

	DesReadDir_resp respuesta;
	uint8_t tam_dir = 0;
	uint16_t off = 0;
	char* mandar;
	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	char* header = malloc(tamano_header);
	char* paquete_recv;
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	size_t tamPaqCache;
	memcached_return retorno;

	pthread_mutex_lock(&s_cache);
	paquete_recv = cache_bajar_info(st_cache, 'd', path_, &tamPaqCache, &retorno);
	pthread_mutex_unlock(&s_cache);

	if (retorno != MEMCACHED_SUCCESS) {

		paquete_send = serializar_path(path_,TIPOREADDIR);

		sem_wait(&socketDisp);

		pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
		pthread_mutex_unlock(&s_vectorPool);

		pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación READDIR - Desde el server - Path %s", path);
		pthread_mutex_unlock(&s_log);

		sockets_send(sock,paquete_send,tamano_paq);


		sockets_recvHeaderNipc(sock,header);

		memcpy(&tipo_recv,header,sizeof(uint8_t));
		if (tipo_recv != TIPOREADDIR) {
			pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock,sock);
			pthread_mutex_unlock(&s_vectorPool);
			sem_post(&socketDisp);
			pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación READDIR - Path %s", path);
			pthread_mutex_unlock(&s_log);
			free(header);
			free(path_);
			return -1;
		}


		memcpy(&tamano_paq,header+1,sizeof(uint16_t));
		paquete_recv = malloc(tamano_paq);
		sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);


		pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock,sock);
		pthread_mutex_unlock(&s_vectorPool);
		sem_post(&socketDisp);

		pthread_mutex_lock(&s_cache);
		retorno = cache_subir_info(st_cache, 'd', path_, paquete_recv, tamano_paq);
		pthread_mutex_unlock(&s_cache);

	} else {


		pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación READDIR - Desde la cache - Path %s", path);
		pthread_mutex_unlock(&s_log);

		tamano_paq = tamPaqCache;

	}

	respuesta = deserializar_Readdir_Rta(tamano_paq, paquete_recv);

	free(paquete_recv);
	
	while(off < respuesta.tamano){

		memcpy(&tam_dir,respuesta.lista_nombres +off,sizeof(uint8_t));
		off = off + sizeof(uint8_t);
		mandar = malloc(tam_dir);
		memcpy(mandar,respuesta.lista_nombres+off,tam_dir);
		off = off + tam_dir;
		filler(buf, mandar, NULL, 0);
		free(mandar);

	};

	free(paquete_send); free(path_);

	free(respuesta.lista_nombres);

	free(header);

	pthread_mutex_lock(&s_log);
	log_debug(logger, "Fin READDIR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	return 0;
}

static int fsc_open(const char *path, struct fuse_file_info *fi) {
	//chequeo si el archivo existe en el rfs, ignoro permisos


	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	char* header = malloc(tamano_header);
	char* paquete_recv;
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	paquete_send = serializar_path(path_,TIPOOPEN);;

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación OPEN - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send); free(path_);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));

	if (tipo_recv != TIPOOPEN) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock,sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación OPEN - Path %s", path);
		pthread_mutex_unlock(&s_log);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock,sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación OPEN - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);
	int respuesta = deserializar_Result_Rta(paquete_recv); //el rfs va a mandar 0 si existe o -EACCES si no existe
	free(paquete_recv);
	return respuesta;

}

static int fsc_release(const char *path, struct fuse_file_info *fi) {
	
	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	paquete_send = serializar_path(path_,TIPORELEASE);;

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación RELEASE - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación RELEASE - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(paquete_send); free(path_);

	//El server no me va a responder
	
	return 0;
}


static int fsc_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
		
	(void) fi;

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1 + sizeof(uint32_t) + sizeof(size_t);
	char* paquete_recv;
	if(size>65535) return -ENOENT;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;


	char* lectura;


	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	paquete_send = serializar_Read_Pedido(path_,offset,size);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación READ - Path %s - Size %d - Offset %d", path, size, offset);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send); free(path_);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));

	if (tipo_recv != TIPOREAD) {

		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación READ - Path %s", path);
		pthread_mutex_unlock(&s_log);

		return -1;
	}

	//Esto no esta andando bien-------------
	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	if (tamano_paq == 0) {
		//paquete_recv = malloc(tamano_paq);
		//sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Lectura Operación READ - Path %s", path);
		pthread_mutex_unlock(&s_log);

		return 0; //Si el tamaño de la lectura es 0 => hubo un error;
	}
	//----------------------------------------

	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);


	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación READ - Path %s - Size %d - Offset %d", path, size, offset);
	pthread_mutex_unlock(&s_log);

	lectura = deserializar_Read_Rta(tamano_paq, paquete_recv);


	memcpy(buf,lectura,tamano_paq);

	free(lectura);
	free(header); free(paquete_recv);

//	sockets_destroy_client(cliente);

//	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
//		return -ENOENT;
//
//	len = strlen(DEFAULT_FILE_CONTENT);
//	if (offset < len) {
//		if (offset + size > len)
//			size = len - offset;
//		memcpy(buf, DEFAULT_FILE_CONTENT + offset, size);
//	} else
//		size = 0;
//	return size;
	return tamano_paq; //Devuelvo el tamaño de la lectura (lectura != 0)
}


static int fsc_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	
	(void) fi;

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1 + sizeof(uint32_t) + sizeof(size_t) + size;
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;
	int respuesta;

	char* path_ ;
	char* buf_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);
	buf_ = malloc(size);
	memcpy(buf_,buf,size);


    paquete_send = serializar_Write_Pedido(path_, size, buf_, offset);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación WRITE - Path %s - Size %d - Offset %d", path, size, offset);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send); free(buf_);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPOWRITE) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación WRITE - Path %s", path);
		pthread_mutex_unlock(&s_log);

		sem_post(&socketDisp);

		free(path_);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	//pthread_mutex_lock(&s_cache);
		//cache_borrarClave(st_cache, 'a', path);
	//pthread_mutex_unlock(&s_cache);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación WRITE - Path %s - Size %d - Offset %d", path, size, offset);
	pthread_mutex_unlock(&s_log);

	free(header);

    respuesta = deserializar_Write_Rta(paquete_recv); //esta funcion va a devolver los bytes q escribio el server
    free(paquete_recv);

    pthread_mutex_lock(&s_cache);
		cache_borrarClave(st_cache, CACHEGETATR, path_);
	pthread_mutex_unlock(&s_cache);

	free(path_);

    return respuesta;
}

static int fsc_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	(void) fi;

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

    paquete_send = serializar_path(path_,TIPOCREATE);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación CREATE - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPOCREATE) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación CREATE - Path %s", path);
		pthread_mutex_unlock(&s_log);

		free(path_);

		return -1;

	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación CREATE - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);

	pthread_mutex_lock(&s_cache);
		borrarCacheDirectorios(path_,st_cache);
	pthread_mutex_unlock(&s_cache);

	free(path_);

	return deserializar_Result_Rta(paquete_recv);
}

static int fsc_unlink(const char *path) {

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	paquete_send = serializar_path(path_,TIPOUNLINK);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación UNLINK - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPOUNLINK) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación UNLINK - Path %s", path);
		pthread_mutex_unlock(&s_log);

		free(path_);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación UNLINK - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);

	pthread_mutex_lock(&s_cache);
		borrarCacheDirectorios(path_,st_cache);
	pthread_mutex_unlock(&s_cache);

	free(path_);

	return deserializar_Result_Rta(paquete_recv); //retornamos el valor que retorna el server 0 exito -1 too mal
}

static int fsc_truncate(const char *path, off_t offset) {

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1 + sizeof(off_t);
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

    paquete_send = serializar_Truncate_Pedido(path_,offset);


	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación TRUNCATE - Path %s - Offset: %d", path, offset);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPOTRUNCATE) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación TRUNCATE - Path %s", path);
		pthread_mutex_unlock(&s_log);

		free(path_);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	//pthread_mutex_lock(&s_cache);
		//cache_borrarClave(st_cache, 'a', path);
	//pthread_mutex_unlock(&s_cache);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación TRUNCATE - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);

	pthread_mutex_lock(&s_cache);
		cache_borrarClave(st_cache, CACHEGETATR, path_);
	pthread_mutex_unlock(&s_cache);

	free(path_);

    return deserializar_Result_Rta(paquete_recv); // segun devuelva el server 0 exito -1 too mal
}

static int fsc_rmdir(const char *path) {

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

    paquete_send = serializar_path(path_, TIPORMDIR);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación RMDIR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPORMDIR) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación RMDIR - Path %s", path);
		pthread_mutex_unlock(&s_log);

		free(path_);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	//pthread_mutex_lock(&s_cache);
		//cache_borrarClave(st_cache, 'a', path);
	//pthread_mutex_unlock(&s_cache);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación RMDIR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);

	pthread_mutex_lock(&s_cache);
		borrarCacheDirectorios(path_,st_cache);
	pthread_mutex_unlock(&s_cache);

	free(path_);

	return deserializar_Result_Rta(paquete_recv);
}

static int fsc_mkdir(const char *path, mode_t mode) {

	char* paquete_send;
	uint16_t tamano_header = sizeof(uint8_t) + sizeof(uint16_t); // = 3
	uint16_t tamano_paq = tamano_header + strlen(path) + 1;
	char* paquete_recv;
	char* header = malloc(tamano_header);
	uint8_t tipo_recv;
	t_socket_client* sock;

	char* path_ ;
	path_ = malloc(strlen(path)+1);
	memcpy(path_,path,strlen(path)+1);

	paquete_send = serializar_path(path_,TIPOMKDIR);

	sem_wait(&socketDisp);

	pthread_mutex_lock(&s_vectorPool);
		sock = obtenerSocket(pool_sock);
	pthread_mutex_unlock(&s_vectorPool);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Operación MKDIR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	sockets_send(sock,paquete_send,tamano_paq);

	free(paquete_send);

	sockets_recvHeaderNipc(sock,header);

	memcpy(&tipo_recv,header,sizeof(uint8_t));
	if (tipo_recv != TIPOMKDIR) {
		pthread_mutex_lock(&s_vectorPool);
			habilitar_socket(pool_sock, sock);
		pthread_mutex_unlock(&s_vectorPool);

		sem_post(&socketDisp);

		pthread_mutex_lock(&s_log);
			log_debug(logger, "Error Operación MKDIR - Path %s", path);
		pthread_mutex_unlock(&s_log);

		free(path_);

		return -1;
	}


	memcpy(&tamano_paq,header+1,sizeof(uint16_t));
	paquete_recv = malloc(tamano_paq);
	sockets_recvPayloadNipc(sock,paquete_recv,tamano_paq);

	pthread_mutex_lock(&s_vectorPool);
		habilitar_socket(pool_sock, sock);
	pthread_mutex_unlock(&s_vectorPool);

	sem_post(&socketDisp);

	pthread_mutex_lock(&s_log);
		log_debug(logger, "Fin Operación MKDIR - Path %s", path);
	pthread_mutex_unlock(&s_log);

	free(header);

	pthread_mutex_lock(&s_cache);
		borrarCacheDirectorios(path_,st_cache);
	pthread_mutex_unlock(&s_cache);

	free(path_);

	return deserializar_Result_Rta(paquete_recv); // retorna segun el server 0 exito -1 too mal
}


/*
 * Esta es la estructura principal de FUSE con la cual nosotros le decimos a
 * biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
 * Como se observa la estructura contiene punteros a funciones.
 */

static struct fuse_operations hello_oper = {
		.getattr = fsc_getattr,
		.readdir = fsc_readdir,
		.open = fsc_open,
		.read = fsc_read,
		.release = fsc_release,
		.read = fsc_read,
		.write = fsc_write,
		.create = fsc_create,
		.unlink = fsc_unlink,
		.truncate = fsc_truncate,
		.rmdir = fsc_rmdir,
		.mkdir = fsc_mkdir,


};


/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};


/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);


//config----------------------------
//	t_config *s_cfg;
//	s_cfg = config_create("configFSC.txt");

	int portL;
	int portR;
	char* ip;
	char* ipr;
	int cantSock;
	char* ipCache;
	int portCache;

	levantar_config(&portL, &portR, &ip, &ipr, &cantSock, &ipCache, &portCache);



//----------------------------------


	//Inicializo semaforos
	pthread_mutex_init(&s_vectorPool,NULL); //NULL para atributos default
	pthread_mutex_init(&s_cache,NULL);
	pthread_mutex_init(&s_log,NULL);
	sem_init(&socketDisp,0,cantSock);


	//Creo el log
	logger = log_create("logFsc.txt","rfc",true,LOG_LEVEL_DEBUG);


	t_socket_client *cliente;
	uint8_t i;

	// Armo el pool de sockets (10 sockets en total);

	pool_sock = queue_create();

	for(i = 0; i <cantSock; i++) {

		cliente = sockets_createClient(ip,portL+i);
		sockets_connect(cliente,ipr,portR);
		if(cliente->state == SOCKETSTATE_CONNECTED) {
			enviarHandshake(cliente);
		}else{
			perror("Error en connect");
			exit(0);
		}
		log_debug(logger, "Conexión desde puerto: %d", portL+i);
		habilitar_socket(pool_sock, cliente);

	}
	//FIN POOL DE SOCKETS


	//Creo cache
	st_cache = cache_crearCache(ipCache,portCache);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --welcome-msg
	// el campo welcome_msg deberia tener el
	// valor pasado
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}
	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &hello_oper, NULL);
}

