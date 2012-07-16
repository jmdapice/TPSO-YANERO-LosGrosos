/*
 * auxCliente.c
 *
 *  Created on: 11/06/2012
 *      Author: utnso
 */


#include "auxCliente.h"
#include "../commons/config.h"

t_socket_client* obtenerSocket(t_queue* pool_sock){

//	int i;
//
//	for(i=0; pool_sock[i].estado == 1; i++);
//	pool_sock[i].estado = 1;
//
//	return &pool_sock[i];

	return (t_socket_client*) queue_pop(pool_sock);

}

void habilitar_socket(t_queue* pool_sock, t_socket_client* sock){

	queue_push(pool_sock, sock);


};


void levantar_config(int* portL, int* portR, char** ip, char** ipr, int* cantSock, char** ipCache, int* portCache){

	t_config *s_cfg;

	s_cfg = config_create("configFSC.txt");

	if(config_has_property(s_cfg,"portClient")) {
		*portL = config_get_int_value(s_cfg,"portClient");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"portServer")) {
		*portR = config_get_int_value(s_cfg,"portServer");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"ipClient")) {
		*ip  = config_get_string_value(s_cfg,"ipClient");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"ipServer")) {
		*ipr = config_get_string_value(s_cfg,"ipServer");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"poolSock")) {
		*cantSock = config_get_int_value(s_cfg,"poolSock");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"ipCache")) {
		*ipCache = config_get_string_value(s_cfg,"ipCache");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

	if(config_has_property(s_cfg,"portCache")) {
		*portCache = config_get_int_value(s_cfg,"portCache");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}

}

void borrarCacheDirectorios(char* path, memcached_st* st_cache){

	PathSeparado dir;

	cache_borrarClave(st_cache, CACHEGETATR, path);
	cache_borrarClave(st_cache, CACHEREADDIR, path);

	dir = separarPathCliente(path);

	cache_borrarClave(st_cache,CACHEGETATR, dir.pathDir);
	cache_borrarClave(st_cache,CACHEREADDIR, dir.pathDir);

	free(dir.nombreArch);
	free(dir.pathDir);

}

PathSeparado separarPathCliente(char *path){

	uint16_t i=0;
	size_t len=strlen(path);
	PathSeparado st_ps;
	for(i=len;path[i]!='/';i--);
	if(i==0){ //es el dir raiz
		st_ps.pathDir=malloc(2);
		st_ps.pathDir[0]='/';
		st_ps.pathDir[1]='\0';
		st_ps.nombreArch=malloc(len-i);
		strncpy(st_ps.nombreArch,path+i+1,len-i-1);
		st_ps.nombreArch[len-i-1]='\0';
	}else{
		st_ps.pathDir=malloc(i+1);
		st_ps.nombreArch=malloc(len-i);
		strncpy(st_ps.pathDir,path,i);
		st_ps.pathDir[i]='\0';
		strncpy(st_ps.nombreArch,path+i+1,len-i-1);
		st_ps.nombreArch[len-i-1]='\0';
	}
	return st_ps;
}
